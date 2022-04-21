# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# you can obtain one at http://mozilla.org/MPL/2.0/.

import os

SRC_DIR = os.path.abspath(
    os.path.join(os.path.dirname(__file__), os.pardir, os.pardir, os.pardir,
                 os.pardir))

SRC_BRAVE_DIR = os.path.join(SRC_DIR, 'brave')

BRAVE_PERF_DIR = os.path.join(SRC_DIR, 'brave', 'tools', 'perf')
