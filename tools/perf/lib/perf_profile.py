# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# you can obtain one at http://mozilla.org/MPL/2.0/.

import subprocess
import logging
import os
import sys

# def DownloadFromGoogleStorage():
#   sys.path.append(os.path.join(_SRC_ROOT, 'third_party', 'depot_tools'))
#   import download_from_google_storage
#   #if os.path.isfile(profile_path):

#   gsutil = download_from_google_storage.Gsutil(
#       download_from_google_storage.GSUTIL_DEFAULT_PATH)
#   gs_path = 'gs://' + args.gs_url_base.strip('/') + '/' + profile_name
#   code = gsutil.call('cp', gs_path, profile_path)

def GetProfilePath(profile_type):
  if profile_type == "empty":
    return None
  if profile_type == "typical":
    return None
  raise RuntimeError("Unknown profile type %s" % profile_type)
