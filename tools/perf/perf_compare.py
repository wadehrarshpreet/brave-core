#!/usr/bin/env python
# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# you can obtain one at http://mozilla.org/MPL/2.0/.
from __future__ import annotations

import sys
import logging
from lib import perf_test_utils, perf_config
import argparse


def main():
  perf_test_utils.FixUpWPRs()

  parser = argparse.ArgumentParser()
  parser.add_argument('--working-directory', required=True, type=str)
  parser.add_argument('--config', required=True, type=str)
  parser.add_argument('--verbose', action='store_true')
  args = parser.parse_args()

  log_level = logging.DEBUG if args.verbose else logging.INFO
  log_format = '%(asctime)s: %(message)s'
  logging.basicConfig(level=log_level, format=log_format)

  json_config = perf_test_utils.LoadConfig(args.config)
  tests_config = json_config['tests']

  common_options = perf_test_utils.CommonOptions.make_local(
      args.working_directory, tests_config)

  configurations = perf_test_utils.ParseConfigurations(
      json_config['configurations'])

  return 0 if perf_test_utils.RunConfigurations(configurations,
                                                common_options) else 1


if __name__ == '__main__':
  sys.exit(main())
