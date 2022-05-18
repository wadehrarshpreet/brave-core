import * as React from 'react'

import {
  TipWrapper,
  Tip,
  Pointer,
  TipAndChildrenWrapper
} from './style'

export interface Props {
  children?: React.ReactNode
  disableHoverEvents?: boolean
  isAddress?: boolean
  isVisible: boolean
  position?: 'left' | 'right'
  text: React.ReactNode
  verticalPosition?: 'above' | 'below'
  horizontalMargin?: string
  pointerPosition?: 'left' | 'right' | 'center'
}

function Tooltip (props: Props) {
  const {
    children,
    disableHoverEvents,
    isAddress,
    isVisible,
    position,
    text,
    verticalPosition = 'below',
    pointerPosition,
    horizontalMargin
  } = props
  const [active, setActive] = React.useState(!!disableHoverEvents)

  const showTip = () => {
    !disableHoverEvents && setActive(true)
  }

  const hideTip = () => {
    !disableHoverEvents && setActive(false)
  }

  const toolTipPointer = React.useMemo(() => (
    <Pointer
      position={pointerPosition ?? 'center'}
      verticalPosition={verticalPosition ?? 'below'}
    />
  ), [position, verticalPosition, pointerPosition])

  const toolTip = React.useMemo(() => active && isVisible && (
    <TipWrapper
      position={position ?? 'center'}
      verticalPosition={verticalPosition ?? 'below'}
      horizontalMargin={horizontalMargin}
    >

      {verticalPosition === 'below' && toolTipPointer}

      <Tip isAddress={isAddress}>
        {text}
      </Tip>

      {verticalPosition === 'above' && toolTipPointer}
    </TipWrapper>
  ), [
    active,
    isVisible,
    position,
    verticalPosition,
    isAddress,
    text,
    horizontalMargin
  ])

  return (
    <TipAndChildrenWrapper
      onMouseEnter={showTip}
      onMouseLeave={hideTip}
    >
      {verticalPosition === 'above' && toolTip}
      {children}
      {verticalPosition === 'below' && toolTip}
    </TipAndChildrenWrapper>
  )
}

Tooltip.defaultProps = {
  isVisible: true
}

export default Tooltip
