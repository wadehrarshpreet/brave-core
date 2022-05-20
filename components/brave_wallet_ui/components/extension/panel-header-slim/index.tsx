import * as React from 'react'

// Styled Components
import {
  HeaderTitle,
  HeaderWrapper,
  TopRow,
  CloseButton
} from './style'
// import { getLocale } from '../../../../common/locale'
import { PanelTypes } from '../../../constants/types'

export interface Props {
  title: string
  action: (path: PanelTypes) => void
}

export default class PanelHeaderSlim extends React.PureComponent<Props> {
  navigate = (path: PanelTypes) => () => {
    this.props.action(path)
  }

  render () {
    const { title } = this.props
    return (
      <HeaderWrapper>
        <TopRow>
          <HeaderTitle>
            {title}
          </HeaderTitle>
          <CloseButton onClick={this.navigate('main')} />
        </TopRow>
      </HeaderWrapper>
    )
  }
}
