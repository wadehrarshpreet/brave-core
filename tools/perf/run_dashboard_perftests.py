#!/usr/bin/env python
# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# you can obtain one at http://mozilla.org/MPL/2.0/.
"""A tool run telemetry perftests and report the results to dashboard


The tool:
1. Download a proper binary.
2. Run a subset of telemetry perftests from the config.
3. Report the result to brave-perf-dashboard.appspot.com
The tools should be run on a special prepared hardware/OS to minimize
the result flakiness.

Example usage:
 vpython3 run_dashboard_perftests.py --working-directory=e:\work\tmp\perf0\
                                     --config=smoke.json
                                     --target v1.36.23
"""
from __future__ import annotations

import sys
import logging
from lib import perf_test_utils, perf_config
import argparse


def main():
  perf_test_utils.FixUpWPRs()

  parser = argparse.ArgumentParser()
  parser.add_argument('--targets',
                      required=True,
                      type=str,
                      help='The targets to test')
  parser.add_argument('--working-directory', required=True, type=str)
  parser.add_argument('--config', required=True, type=str)
  parser.add_argument('--no-report', action='store_true')
  parser.add_argument('--report-only', action='store_true')
  parser.add_argument('--report-on-failure', action='store_true')
  parser.add_argument('--local-run', action='store_true')
  parser.add_argument('--verbose', action='store_true')
  args = parser.parse_args()

  log_level = logging.DEBUG if args.verbose else logging.INFO
  log_format = '%(asctime)s: %(message)s'
  logging.basicConfig(level=log_level, format=log_format)

  json_config = perf_test_utils.LoadConfig(args.config)
  targets = args.targets.split(',')
  base_configuration = perf_config.PerfConfiguration(
      json_config['configuration'])

  tests_config = json_config['tests']

  common_options = perf_test_utils.CommonOptions.from_args(args, tests_config)

  configurations = perf_test_utils.SpawnConfigurationsFromTargetList(
      targets, base_configuration)

  return 0 if perf_test_utils.RunConfigurations(configurations,
                                                common_options) else 1


if __name__ == '__main__':
  sys.exit(main())
