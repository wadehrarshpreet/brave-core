# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# you can obtain one at http://mozilla.org/MPL/2.0/.

import logging
import os
import sys
import subprocess
import hashlib
import uuid
import shutil

from zipfile import ZipFile

from lib import path_util

sys.path.append(os.path.join(path_util.SRC_DIR, 'third_party', 'depot_tools'))
import download_from_google_storage


# To upload call: upload_to_google_storage.py some_profile.zip -b brave-telemetry
def DownloadFromGoogleStorage(sha1, output_path):

  gsutil = download_from_google_storage.Gsutil(
      download_from_google_storage.GSUTIL_DEFAULT_PATH)
  gs_path = 'gs://' + path_util.BRAVE_PERF_BUCKET + '/' + sha1
  logging.info(f'Download profile from {gs_path} to {output_path}')
  exit_code = gsutil.call('cp', gs_path, output_path)
  if exit_code:
    raise RuntimeError(f'Failed to download: {gs_path}')


def GetProfilePath(profile, binary, work_directory):
  if profile == 'clean':
    return None

  binary_path_hash = hashlib.sha1(binary.encode("utf-8")).hexdigest()[:6]
  profile_id = profile + '-' + binary_path_hash


  if not hasattr(GetProfilePath, 'profiles'):
    GetProfilePath.profiles = {}

  if profile in GetProfilePath.profiles:
    return GetProfilePath.profiles[profile_id]

  dir = None
  if os.path.isdir(profile):  # local profile
    dir = os.path.join(work_directory, 'profiles',
                       uuid.uuid4().hex.upper()[0:6])
    shutil.copytree(profile, dir)
    RebaseProfile(binary, dir)
  else:
    zip_path = os.path.join(path_util.BRAVE_PERF_PROFILE_DIR, profile + '.zip')
    zip_path_sha1 = os.path.join(path_util.BRAVE_PERF_PROFILE_DIR,
                               profile + '.zip.sha1')

    if not os.path.isfile(zip_path_sha1):
      raise RuntimeError(f'Unknown profile, file {zip_path_sha1} not found')

    sha1 = None
    with open(zip_path_sha1, 'r') as sha1_file:
      sha1 = sha1_file.read().rstrip()
    logging.debug(f'Expected hash {sha1} for profile {profile}')
    if not sha1:
      raise RuntimeError(f'Bad sha1 in {zip_path_sha1}')

    if not os.path.isfile(zip_path):
      DownloadFromGoogleStorage(sha1, zip_path)
    else:
      current_sha1 = download_from_google_storage.get_sha1(zip_path)
      if current_sha1 != sha1:
        logging.info(f'Profile needs to be updated. Current hash {current_sha1}, expected {sha1}')
        DownloadFromGoogleStorage(sha1, zip_path)
    dir = os.path.join(work_directory, 'profiles', profile_id + '-' + sha1)

    if not os.path.isdir(dir):
      os.makedirs(dir)
      logging.info(f'Create temp profile dir {dir} for profile {profile}')
      zipfile = ZipFile(zip_path)
      zipfile.extractall(dir)
      RebaseProfile(binary, dir)

  logging.info(f'Use temp profile dir {dir} for profile {profile}')
  GetProfilePath.profiles[profile_id] = dir
  return dir


def RebaseProfile(binary, profile_directory, extra_browser_args=[]):
  logging.info(f'Rebasing dir {profile_directory} using binary {binary}')
  args = [
      sys.executable,
      os.path.join(path_util.SRC_DIR, 'tools', 'perf', 'run_benchmark'),
      'system_health.common_desktop'
  ]
  args.append(f'--story=load:media:youtube:2018')
  args.append('--browser=exact')
  args.append(f'--browser-executable={binary}')

  args.append(f'--profile-dir={profile_directory}')

  extra_browser_args.append('--update-source-profile')
  # TODO: add  is_chromium, see _GetVariationsBrowserArgs
  extra_browser_args.append('--use-brave-field-trial-config')

  args.append('--extra-browser-args=' + ' '.join(extra_browser_args))

  #TODO: add output

  logging.debug('Run binary:' + ' '.join(args))

  result = subprocess.run(args,
                          stdout=subprocess.PIPE,
                          stderr=subprocess.STDOUT,
                          cwd=os.path.join(path_util.SRC_DIR, 'tools', 'perf'))
  if result.returncode != 0:
    logging.error(result.stdout.decode('utf-8'))
    return False
  else:
    logging.debug(result.stdout.decode('utf-8'))
    return True
