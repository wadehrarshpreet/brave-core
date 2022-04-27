# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# you can obtain one at http://mozilla.org/MPL/2.0/.

import logging

class PerfConfiguration(object):
  profile = 'empty'
  extra_browser_args = []


class PerfConfigurationWithTarget(PerfConfiguration):
  target = None

  def __init__(self, config_dict):
    for key in config_dict:
      if not hasattr(self, key):
        raise RuntimeError('Unexpected %s in configuration', key)
      key = key.replace('-', '_')
      setattr(self, key, config_dict[key])
    if not self.target:
      raise RuntimeError('target should be specified in configuration')


class PerfDashboardConfiguration(PerfConfiguration):
  dashboard_bot_name = None
  extra_benchmark_args = []
  chromium = False

  def __init__(self, config_dict):
    if not config_dict:
      raise RuntimeError('Missing \'configuration\'')
    for key in config_dict:
      key_ = key.replace('-', '_')
      if not hasattr(self, key_):
        raise RuntimeError('Unexpected %s in configuration' % key)
      setattr(self, key_, config_dict[key])
    if not self.dashboard_bot_name:
      raise RuntimeError(
          'dashboard_bot_name should be specified in configuration')
