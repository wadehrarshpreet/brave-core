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
  horizontalMarginPx?: number
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
    horizontalMarginPx
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
      position={pointerPosition ?? position ?? 'center'}
      verticalPosition={verticalPosition ?? 'below'}
    />
  ), [position, verticalPosition, pointerPosition])

  const toolTip = React.useMemo(() => active && isVisible && (
    <TipWrapper
      position={position ?? 'center'}
      verticalPosition={verticalPosition ?? 'below'}
      horizontalMarginPx={horizontalMarginPx}
    >

      {verticalPosition === 'below' && toolTipPointer}

      <Tip
        isAddress={isAddress}
        position={position ?? 'center'}
        verticalPosition={verticalPosition ?? 'below'}
      >
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
    horizontalMarginPx
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
