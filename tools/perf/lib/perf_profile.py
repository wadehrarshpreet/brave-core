# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# you can obtain one at http://mozilla.org/MPL/2.0/.

import logging
import os
import sys

from zipfile import ZipFile

from lib import path_util

sys.path.append(os.path.join(path_util.SRC_DIR, 'third_party', 'depot_tools'))
import download_from_google_storage


# To upload call: upload_to_google_storage.py some_profile.zip -b brave-telemetry
def DownloadFromGoogleStorage(sha1, output_path):

  gsutil = download_from_google_storage.Gsutil(
      download_from_google_storage.GSUTIL_DEFAULT_PATH)
  gs_path = 'gs://' + path_util.BRAVE_PERF_BUCKET + '/' + sha1
  logging.info('Download profile from %s to %s', gs_path, output_path)
  exit_code = gsutil.call('cp', gs_path, output_path)
  if exit_code:
    raise RuntimeError('Failed to download: "%s"' % gs_path)


def GetProfilePath(profile, work_directory):
  logging.debug(f'profile {profile}')
  if os.path.isdir(profile):  # local profile
    return profile

  if profile == 'empty':
    return None

  if not hasattr(GetProfilePath, 'profiles'):
    GetProfilePath.profiles = {}

  if profile in GetProfilePath.profiles:
    return GetProfilePath.profiles[profile]

  zip_path = os.path.join(path_util.BRAVE_PERF_PROFILE_DIR,
                          profile + '.zip')
  zip_path_sha1 = os.path.join(path_util.BRAVE_PERF_PROFILE_DIR,
                               profile + '.zip.sha1')

  if not os.path.isfile(zip_path_sha1):
    raise RuntimeError('Unknown profile, file %s not found' %
                       zip_path_sha1)

  sha1 = None
  with open(zip_path_sha1, 'r') as sha1_file:
    sha1 = sha1_file.read().rstrip()
  logging.debug('Expected hash %s for profile %s', sha1, profile)
  if not sha1:
    raise RuntimeError('Bad sha1 in ' % zip_path_sha1)

  if not os.path.isfile(zip_path):
    DownloadFromGoogleStorage(sha1, zip_path)
  else:
    current_sha1 = download_from_google_storage.get_sha1(zip_path)
    if current_sha1 != sha1:
      logging.info('Profile needs to be updated. Current hash%s, expected %s',
                   current_sha1, sha1)
      DownloadFromGoogleStorage(sha1, zip_path)

  dir = os.path.join(work_directory, 'profiles', profile + sha1)
  if not os.path.isdir(dir):
    os.makedirs(dir)
    logging.info('Create temp profile dir %s for profile %s', dir, profile)
    zipfile = ZipFile(zip_path)
    zipfile.extractall(dir)
  else:
    logging.info('Use temp profile dir %s for profile %s', dir, profile)

  GetProfilePath.profiles[profile] = dir
  return dir
