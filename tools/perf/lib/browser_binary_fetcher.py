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
from urllib.request import urlopen
from io import BytesIO
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
  return version_string.split('.')


def GetNearestChromiumVersionAndUrl(tag):
  chrome_versions = {}
  with open(CHROME_RELEASES_JSON, 'r') as config_file:
    chrome_versions = json.load(config_file)

  args = ['git', 'fetch', 'origin', (f'refs/tags/{tag}')]
  logging.debug('Run binary:' + ' '.join(args))
  subprocess.check_call(args, cwd=path_util.BRAVE_SRC_DIR)
  package_json = json.loads(
      subprocess.check_output(['git', 'show', 'FETCH_HEAD:package.json'],
                              cwd=path_util.BRAVE_SRC_DIR))
  requested_version = package_json['config']['projects']['chrome']['tag']
  logging.debug(f'Got requested_version: {requested_version}')

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
    logging.info(f'Use chromium version {best_candidate} for requested {requested_version}')
    return string_version, chrome_versions[string_version]['url']

  logging.error(f'No chromium version found for {requested_version}')
  return None, None


def DownloadArchiveAndUnpack(output_directory, url):
  logging.info(f'Downloading archive {url}')
  resp = urlopen(url)
  zipfile = ZipFile(BytesIO(resp.read()))
  zipfile.extractall(output_directory)
  return os.path.join(output_directory, path_util.GetBinaryPath(output_directory))


def DownloadWinInstallerAndExtract(out_dir, url, expected_install_path, binary):
  if not os.path.exists(out_dir):
    os.makedirs(out_dir)
  installer_filename = os.path.join(out_dir, os.pardir, 'temp_installer.exe')
  logging.info(f'Downloading {url}')
  f = urlopen(url)
  data = f.read()
  with open(installer_filename, 'wb') as output_file:
    output_file.write(data)
  logging.info(f'Run installer {installer_filename}')
  subprocess.check_call([installer_filename, '--chrome-sxs'])
  try:
    subprocess.check_call(['taskkill.exe', '/f', '/im', binary])
  except subprocess.CalledProcessError:
    logging.info(f'failed to kill {binary}')

  if not os.path.exists(expected_install_path):
    raise RuntimeError(f'No files found in {expected_install_path}')

  full_version = None
  logging.info(f'Copy files to {out_dir}')
  copy_tree(expected_install_path, out_dir)
  for file in os.listdir(expected_install_path):
    #TODO: check brave tag? file.endswith(tag[2:])
    if re.match('\d+\.\d+\.\d+.\d+', file):
      assert (full_version == None)
      full_version = file
  assert (full_version != None)
  logging.info(f'Detected version {full_version}')
  setup_filename = os.path.join(expected_install_path, full_version,
                                'Installer', 'setup.exe')
  logging.info(f'Run uninstall {setup_filename}')
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
  logging.debug(f'Parsed tag: {m.group(1)}, location : {m.group(2)}')
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
      raise RuntimeError(f'Failed to parse tag "{tag}"')

    # nightly < v1.35 has a broken .zip archive
    if int(m.group(1)) == 1 and int(m.group(2)) < 35:
      return PrepareBinaryByUrl(out_dir, BRAVE_NIGHTLY_WIN_INSTALLER_URL % tag,
                                False)
    else:
      import sys
      if sys.platform == 'win32':
        platform = 'win32-x64'
      if sys.platform == 'darwin':
        platform = 'darwin-arm64'
      #hdiutil attach -nobrowse -noautoopen
      return PrepareBinaryByUrl(out_dir,
                                BRAVE_NIGHTLY_URL % (tag, tag, platform), False)


def PrepareBinary(out_dir, tag, location, is_chromium):
  if location:  # local binary
    if os.path.exists(location):
      return location
    else:
      raise RuntimeError(f'{location} doesn\'t exist')
  elif location and location.startswith('http'):  # url
    return PrepareBinaryByUrl(out_dir, location, is_chromium)
  else:
    return PrepareBinaryByTag(out_dir, tag, is_chromium)
