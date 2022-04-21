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
 vpython run_brave_perftests.py --tags v1.36.23
                                --configuration_name=test-agent
                                --work_directory=e:\work\tmp\perf0\
                                --extra_args="--use-live-sites"
"""
import os
import subprocess
import json
import sys
import logging
import shutil
import uuid
import tempfile
from lib import path_util
from lib import browser_binary_fetcher
import argparse


test_config_file_path = os.path.join(path_util.BRAVE_PERF_DIR, 'perftest_config.json')

json_config = {}
with open(test_config_file_path, 'r') as config_file:
  json_config = json.load(config_file)

# Workaround to add our wpr files
page_set_data_dir = os.path.join(path_util.BRAVE_PERF_DIR, 'page_sets_data')
chromium_page_set_data_dir = os.path.join(path_util.SRC_DIR, 'tools', 'perf', 'page_sets',
                                          'data')
for item in os.listdir(page_set_data_dir):
  shutil.copy(os.path.join(page_set_data_dir, item), chromium_page_set_data_dir)


# Returns pair [revision_number, sha1]. revision_number is a number "primary"
# commits from the begging to `revision`.
# Use this to get the commit from a revision number:
# git rev-list --topo-order --first-parent --reverse origin/master
# | head -n <rev_num> | tail -n 1 | git log -n 1 --stdin
def GetRevisionNumberAndHash(revision):
  brave_dir = os.path.join(path_util.SRC_DIR, 'brave')
  subprocess.check_call(['git', 'fetch', 'origin', revision], cwd=brave_dir)
  hash = subprocess.check_output(['git', 'rev-parse', 'FETCH_HEAD'],
                                 cwd=brave_dir).rstrip()
  rev_number_args = [
      'git', 'rev-list', '--topo-order', '--first-parent', '--count',
      'FETCH_HEAD'
  ]
  rev_number = subprocess.check_output(rev_number_args, cwd=brave_dir).rstrip()
  return [rev_number, hash]


def RunTest(binary, config, out_dir, is_ref, extra_args=[]):
  args = [
      sys.executable,
      os.path.join(path_util.SRC_DIR, 'testing', 'scripts', 'run_performance_tests.py'),
      os.path.join(path_util.SRC_DIR, 'tools', 'perf', 'run_benchmark')
  ]
  benchmark = config['benchmark']
  if is_ref:
    benchmark += '.reference'
  args.append('--benchmarks=%s' % benchmark)
  args.append('--browser=exact')
  args.append('--browser-executable=%s' % binary)
  args.append('--isolated-script-test-output=%s' % out_dir + '\\' +
              config['benchmark'] + '\\output.json')
  args.append('--pageset-repeat=%d' % config['pageset_repeat'])
  if 'stories' in config:
    for story in config['stories']:
      args.append('--story=' + story)

  args.extend(extra_args)

  logging.info(' '.join(args))

  subprocess.check_call(args, cwd=os.path.join(path_util.SRC_DIR, 'tools', 'perf'))


def ReportToDashboard(product, is_ref, configuration_name, revision,
                      output_dir):
  args = [
      sys.executable,
      os.path.join(path_util.SRC_DIR, 'tools', 'perf', 'process_perf_results.py')
  ]
  args.append('--configuration-name=%s' % configuration_name)
  args.append('--task-output-dir=%s' % output_dir)
  args.append('--output-json=%s' % os.path.join(output_dir, 'results.json'))

  [revision_number, git_hash] = GetRevisionNumberAndHash(revision)
  build_properties = {}
  build_properties['bot_id'] = 'test_bot'
  if product == 'brave':
    build_properties['builder_group'] = 'brave.perf'
  elif product == 'chromium':
    build_properties[
        'builder_group'] = 'brave.perf' if is_ref else 'chromium.perf'
  else:
    raise RuntimeError('bad product name ' + product)

  build_properties['parent_builder_group'] = 'chromium.linux'
  build_properties['parent_buildername'] = 'Linux Builder'
  build_properties['recipe'] = 'chromium'
  build_properties['slavename'] = 'test_bot'
  build_properties['buildername'] = product + '/' + revision
  build_properties['buildnumber'] = "001"

  build_properties[
      'got_revision_cp'] = 'refs/heads/main@{#%s}' % revision_number
  build_properties['got_v8_revision'] = revision_number
  build_properties['got_webrtc_revision'] = revision_number
  build_properties['git_revision'] = git_hash
  build_properties_serialized = json.dumps(build_properties)
  args.append('--build-properties=%s' % build_properties_serialized)
  subprocess.check_call(args)


def TestBinary(product, revision, binary, output_dir, is_ref, args):
  failedLogs = []
  has_failure = False
  for test_config in json_config['tests']:
    benchmark = test_config['benchmark']
    if not args.report_only:
      try:
        RunTest(binary, test_config, output_dir, is_ref, args.extra_args)
      except subprocess.CalledProcessError:
        has_failure = True
        error = 'Test case %s failed on revision %s' % (benchmark, revision)
        error += '\nLogs: ' + os.path.join(output_dir, benchmark, benchmark,
                                           'benchmark_log.txt')
        logging.error(error)
        failedLogs.append(error)
  try:
    if has_failure and not args.report_on_failure:
      error = 'Skip reporting because errors for binary ' + binary
      logging.error(error)
      failedLogs.append(error)
    elif args.skip_report:
      logging.info('skip reporting because report==False')
    else:
      ReportToDashboard(product, is_ref, args.configuration_name, revision,
                        output_dir)

  except subprocess.CalledProcessError:
    has_failure = True
    error = 'Reporting ' + revision + ' failed'
    logging.error(error)
    failedLogs.append(error)
  return [not has_failure, failedLogs]


failedLogs = []
has_failure = False

parser = argparse.ArgumentParser()
parser.add_argument('--target',
                    required=True,
                    action='append',
                    help='The target to test')
parser.add_argument('--work-directory', required=True, type=str)
parser.add_argument('--configuration-name', required=True, type=str)
parser.add_argument('--chromium', action='store_true')
parser.add_argument('--extra-args', action='append', default=[])
parser.add_argument('--skip-report', action='store_true')
parser.add_argument('--report-only', action='store_true')
parser.add_argument('--report-on-failure', action='store_true')
parser.add_argument('--verbose', action='store_true')
args = parser.parse_args()

log_level = logging.DEBUG if args.verbose else logging.INFO
log_format = '%(asctime)s - %(levelname)s - %(funcName)s: %(message)s'
logging.basicConfig(level=log_level, format=log_format)

binaries = {}
tags = {}
out_dir = {}
for target in args.target:
  tag = browser_binary_fetcher.GetTagForTarget(target)
  assert(tag != None)
  tags[target] = tag
  out_dir[target] = os.path.join(args.work_directory, tag)

  if not args.report_only:
    shutil.rmtree(out_dir[target], True)
    binaries[target] = browser_binary_fetcher.PrepareBinary(out_dir[target], target, args.chromium)
  logging.info("configuration %s : %s directory %s", tag, binaries[target], out_dir[target])

for target in args.target:
  tag = tags[target]
  out_dir = out_dir[target]

  product = 'chromium' if args.chromium else 'brave'
  is_ref = args.chromium

  [binary_success,
   binary_logs] = TestBinary(product, 'refs/tags/' + tag, binaries[target],
                             os.path.join(out_dir, 'results'), is_ref, args)
  if not binary_success:
    has_failure = True
    failedLogs.extend(binary_logs)

logging.info('\nSummary:\n')
if has_failure:
  logging.error('\n'.join(failedLogs))
  logging.error('Got %d errors!' % len(failedLogs))
else:
  logging.info('OK')
