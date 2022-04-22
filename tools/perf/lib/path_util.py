# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# you can obtain one at http://mozilla.org/MPL/2.0/.

import os
import sys

SRC_DIR = os.path.abspath(
    os.path.join(os.path.dirname(__file__), os.pardir, os.pardir, os.pardir,
                 os.pardir))

BRAVE_SRC_DIR = os.path.join(SRC_DIR, 'brave')

BRAVE_PERF_DIR = os.path.join(SRC_DIR, 'brave', 'tools', 'perf')

BRAVE_DEPOT_TOOLS_DIR = os.path.join(BRAVE_PERF_DIR, 'vendor', 'depot_tools')

VPYTHON_2_PATH = os.path.join(
    BRAVE_DEPOT_TOOLS_DIR,
    'vpython3.bat' if sys.platform == 'win32' else 'vpython3')
BRAVE_PERF_BUCKET = 'brave-telemetry'

BRAVE_PERF_PROFILE_DIR = os.path.join(BRAVE_PERF_DIR, 'profiles')
