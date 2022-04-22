# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# you can obtain one at http://mozilla.org/MPL/2.0/.

import subprocess
import logging
import os
import sys
import tempfile
import hashlib

from io import StringIO
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

def GetProfilePath(profile_type, work_directory):
  if profile_type == 'empty':
    return None

  if not hasattr(GetProfilePath, 'profiles'):
    GetProfilePath.profiles = {}

  if profile_type in GetProfilePath.profiles:
    return GetProfilePath.profiles[profile_type].name

  zip_path = os.path.join(path_util.BRAVE_PERF_PROFILE_DIR,
                          profile_type + '.zip')
  zip_path_sha1 = os.path.join(path_util.BRAVE_PERF_PROFILE_DIR,
                                          profile_type + '.zip.sha1')

  if not os.path.isfile(zip_path_sha1):
    raise RuntimeError('Unknown profile type, file %s not found' % zip_path_sha1)

  sha1 = None
  with open(zip_path_sha1, 'r') as sha1_file:
    sha1 = sha1_file.read().rstrip()
  logging.info('Expected hash %s for profile %s', sha1, profile_type)
  if not sha1:
    raise RuntimeError('Bad sha1 in ' % zip_path_sha1)

  if not os.path.isfile(zip_path):
    DownloadFromGoogleStorage(sha1, zip_path)
  else:
    current_sha1 = str(download_from_google_storage.get_sha1(zip_path).encode())
    if current_sha1 != sha1:
      logging.info('Hash mismatch current %s, expected %s', current_sha1, sha1)
      DownloadFromGoogleStorage(sha1, zip_path)

  tmp_dir = tempfile.TemporaryDirectory(prefix='profile_' + profile_type,
                                        dir=work_directory)
  zipfile = ZipFile(zip_path)
  zipfile.extractall(tmp_dir.name)

  logging.info('Use temp profile dir %s for profile %s', tmp_dir,
               profile_type)
  return None
  # GetProfilePath.profiles[profile_type] = tmp_dir
  # return tmp_dir.name
