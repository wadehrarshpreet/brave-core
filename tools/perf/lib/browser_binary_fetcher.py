# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# you can obtain one at http://mozilla.org/MPL/2.0/.

import subprocess
import logging
import json
import os
import shutil
import re
from urllib2 import urlopen
from StringIO import StringIO
from zipfile import ZipFile
from distutils.dir_util import copy_tree

from lib import path_util

BRAVE_NIGHTLY_WIN_INSTALLER_URL = 'https://github.com/brave/brave-browser/releases/download/%s/BraveBrowserStandaloneSilentNightlySetup.exe'
BRAVE_NIGHTLY_URL = 'https://github.com/brave/brave-browser/releases/download/%s/brave-%s-%s.zip'

CHROME_RELEASES_JSON = os.path.join(path_util.BRAVE_PERF_DIR,
                                    'chrome_releases.json')


def GetBraveNightlyUrl(tag, platform):
  return BRAVE_NIGHTLY_URL % (tag, tag, platform)


def ParseVersion(version_string):
  return map(int, version_string.split('.'))


def GetNearestChromiumVersionAndUrl(tag):
  chrome_versions = {}
  with open(CHROME_RELEASES_JSON, 'r') as config_file:
    chrome_versions = json.load(config_file)

  subprocess.check_call(['git', 'fetch', 'origin', ('refs/tags/%s' % tag)],
                        cwd=path_util.SRC_BRAVE_DIR)
  package_json = json.loads(
      subprocess.check_output(['git', 'show', 'FETCH_HEAD:package.json'],
                              cwd=path_util.SRC_BRAVE_DIR))
  requested_version = package_json['config']['projects']['chrome']['tag']

  parsed_requested_version = ParseVersion(requested_version)
  best_candidate = None
  for version in chrome_versions:
    parsed_version = ParseVersion(version)
    if parsed_version[0] == parsed_requested_version[
        0] and parsed_version >= parsed_requested_version:
      if not best_candidate or best_candidate > parsed_version:
        best_candidate = parsed_version

  if best_candidate:
    string_version = '.'.join(map(str, best_candidate))
    logging.info('Use chromium version %s for requested %s ' %
                 (best_candidate, requested_version))
    return string_version, chrome_versions[string_version]['url']

  logging.error('No chromium version found for %s' % requested_version)
  return None, None


def DownloadArchiveAndUnpack(output_directory, url):
  logging.info('Downloading archive %s' % url)
  resp = urlopen(url)
  zipfile = ZipFile(StringIO(resp.read()))
  zipfile.extractall(output_directory)
  return os.path.join(output_directory, 'brave.exe')


def DownloadWinInstallerAndExtract(out_dir, url, expected_install_path, binary):
  if not os.path.exists(out_dir):
    os.makedirs(out_dir)
  installer_filename = os.path.join(out_dir, os.pardir, 'temp_installer.exe')
  logging.info('Downloading %s' % url)
  f = urlopen(url)
  data = f.read()
  with open(installer_filename, 'wb') as output_file:
    output_file.write(data)
  logging.info('Run installer %s' % installer_filename)
  subprocess.check_call([installer_filename, '--chrome-sxs'])
  try:
    subprocess.check_call(['taskkill.exe', '/f', '/im', binary])
  except subprocess.CalledProcessError:
    logging.info('failed to kill %s' % binary)

  if not os.path.exists(expected_install_path):
    raise RuntimeError('No files found in %s' % expected_install_path)

  full_version = None
  logging.info('Copy files to %s' % out_dir)
  copy_tree(expected_install_path, out_dir)
  for file in os.listdir(expected_install_path):
    #TODO: check brave tag? file.endswith(tag[2:])
    if re.match('\d+\.\d+\.\d+.\d+', file):
      assert (full_version == None)
      full_version = file
  assert (full_version != None)
  logging.info('Detected version %s' % full_version)
  setup_filename = os.path.join(expected_install_path, full_version,
                                'Installer', 'setup.exe')
  logging.info('Run uninstall %s' % setup_filename)
  try:
    subprocess.check_call(
        [setup_filename, '--uninstall', '--force-uninstall', '--chrome-sxs'])
  except subprocess.CalledProcessError:
    logging.info('setup.exe returns non-zero code.')

  shutil.rmtree(expected_install_path, True)
  return os.path.join(out_dir, binary)


def PrepareBinaryByUrl(out_dir, url, is_chromium):
  if url.endswith('.zip'):
    return DownloadArchiveAndUnpack(out_dir, url)
  if is_chromium:
    install_path = os.path.join(os.path.expanduser('~'), 'AppData', 'Local',
                                'Google', 'Chrome SxS', 'Application')
    return DownloadWinInstallerAndExtract(out_dir, url, install_path,
                                          'chrome.exe')
  else:
    install_path = os.path.join(os.path.expanduser('~'), 'AppData', 'Local',
                                'BraveSoftware', 'Brave-Browser-Nightly',
                                'Application')
    return DownloadWinInstallerAndExtract(out_dir, url, install_path,
                                          'brave.exe')


def ParseTarget(target):
  m = re.match('^(v\d+\.\d+\.\d+)(?::(.+)|$)', target)
  if not m:
    return None, target
  return m.group(1), m.group(2)


def PrepareBinaryByTag(out_dir, tag, is_chromium):
  if is_chromium:
    [chromium_version, url] = GetNearestChromiumVersionAndUrl(tag)
    if not url:
      raise RuntimeError('Failed to find nearest chromium binary for %s' % tag)
    return PrepareBinaryByUrl(out_dir, url, True)

  else:  #is_brave
    m = re.match('^v(\d+)\.(\d+)\.\d+$', tag)
    if not m:
      raise RuntimeError('Failed to parse tag "%s"' % tag)

    # nightly < v1.35 has a broken .zip archive
    if m.group(1) == 1 and m.group(2) < 35:
      return PrepareBinaryByUrl(out_dir, BRAVE_NIGHTLY_WIN_INSTALLER_URL % tag,
                                False)
    else:
      platform = 'win32-x64'
      return PrepareBinaryByUrl(out_dir,
                                BRAVE_NIGHTLY_URL % (tag, tag, platform), False)


def PrepareBinary(out_dir, target, is_chromium):
  tag, location = ParseTarget(target)
  if location and os.path.exists(location):  # local binary
    return location
  elif location and location.startswith('http'):  # url
    return PrepareBinaryByUrl(out_dir, location, is_chromium)
  else:
    return PrepareBinaryByTag(out_dir, tag, is_chromium)


def GetTagForTarget(target):
  tag, _ = ParseTarget(target)
  return tag
