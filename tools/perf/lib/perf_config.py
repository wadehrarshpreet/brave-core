# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# you can obtain one at http://mozilla.org/MPL/2.0/.

import logging
from lib import browser_binary_fetcher


class PerfConfiguration:
  tag = None
  location = None
  label = None
  profile = 'empty'
  extra_browser_args = []
  extra_benchmark_args = []
  chromium = False
  dashboard_bot_name = None

  def __init__(self, config_dict):
    for key in config_dict:
      if key == 'target':
        self.tag, self.location = browser_binary_fetcher.ParseTarget(
            config_dict[key])
        continue
      key_ = key.replace('-', '_')
      if not hasattr(self, key_):
        raise RuntimeError(f'Unexpected {key} in configuration')
      setattr(self, key_, config_dict[key])
