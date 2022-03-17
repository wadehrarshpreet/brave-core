/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import PrivateTab from '../containers/privateTab'
import { Dispatch } from 'redux'
import { DisclaimerDialog, SearchBox, BadgeTor } from '../components/private'
import { getLocale } from '$web-common/locale'
import store from '../store'
import { getNewTabData } from './default/data/storybookState'
import { getActionsForDispatch } from '../api/getActions'

export default {
  title: 'New Tab/PrivateNTP',
  decorators: [
    (Story: any) => {
      return (
        <Story />
      )
    }
  ]
}

const doNothingDispatch: Dispatch = (action: any) => action

function getActions () {
  return getActionsForDispatch(doNothingDispatch)
}

export const _PrivateTab = () => {
  const state = store.getState()
  const newTabData = getNewTabData(state.newTabData)

  return (
    <PrivateTab
      newTabData={newTabData}
      actions={getActions()}
    />
  )
}

export const _DisclaimerDialog = () => {
  return (
    <DisclaimerDialog>
      <p>{getLocale('headerText1')}</p>
    </DisclaimerDialog>
  )
}

export const _SearchBox = () => {
  return (<SearchBox />)
}

export const _BadgeTor = () => {
  return (<BadgeTor isLoading={false} isConnected={true} progress="20" />)
}
