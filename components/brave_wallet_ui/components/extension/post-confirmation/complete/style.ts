import styled from 'styled-components'

import SuccessSvg from '../../../../assets/svg-icons/success-circle-icon.svg'
import { TransactionStatusIcon, TransactionStatusText } from '../common/style'

export const SuccessIcon = styled(TransactionStatusIcon)`
  background: url(${SuccessSvg});
`

export const Title = styled(TransactionStatusText)`
  color: ${(p) => p.theme.color.text01};
`
