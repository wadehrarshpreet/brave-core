import * as React from 'react'
import { useSelector } from 'react-redux'

// Constants
import { WalletState } from '../../../../constants/types'

// Hooks
import { useExplorer } from '../../../../common/hooks'

// Utils
import { getLocale } from '$web-common/locale'

// Styled components
import { Panel } from '../..'
import {
  ConfirmationsNumber,
  ConfirmingIcon,
  ConfirmingText,
  Title
} from './style'
import {
  ButtonRow,
  DetailButton,
  LinkIcon,
  TransactionStatusDescription
} from '../common/style'

interface Props {
  headerTitle: string
  confirmations: number
  confirmationsNeeded: number
  onClose: () => void
}

const TransactionConfirming = (props: Props) => {
  const {
    headerTitle,
    confirmations,
    confirmationsNeeded,
    onClose
  } = props

  // redux
  const {
    selectedNetwork
  } = useSelector((state: { wallet: WalletState }) => state.wallet)
  const onClickViewOnBlockExplorer = useExplorer(selectedNetwork)

  return (
    <Panel
      navAction={onClose}
      title={headerTitle}
      headerStyle='slim'
    >
      <ConfirmingIcon>
        <ConfirmingText>
          {getLocale('braveWalletTransactionConfirmingText')}
        </ConfirmingText>
        <ConfirmationsNumber>
          {confirmations} / {confirmationsNeeded}
        </ConfirmationsNumber>
      </ConfirmingIcon>

      <Title>
        {getLocale('braveWalletTransactionConfirmingTitle')}
      </Title>

      <TransactionStatusDescription>
        {getLocale('braveWalletTransactionConfirmingDescription')}
      </TransactionStatusDescription>
      <ButtonRow>
        <DetailButton
          onSubmit={onClickViewOnBlockExplorer('tx', '0x1')}
        >
          {getLocale('braveWalletTransactionExplorer')}
        </DetailButton>
        <LinkIcon />
      </ButtonRow>
    </Panel>
  )
}

export default TransactionConfirming
