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
  SubmittedIcon,
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
  onClose: () => void
}

const TransactionSubmitted = (props: Props) => {
  const {
    headerTitle,
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
      <SubmittedIcon />
      <Title>{getLocale('braveWalletTransactionSubmittedTitle')}</Title>
      <TransactionStatusDescription>
        {getLocale('braveWalletTransactionSubmittedDescription')}
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

export default TransactionSubmitted
