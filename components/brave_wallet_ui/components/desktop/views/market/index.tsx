// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useSelector, useDispatch } from 'react-redux'
import { useParams } from 'react-router-dom'
import { useHistory } from 'react-router'

// Constants
import {
  AddAccountNavTypes,
  AssetFilterOption,
  BraveWallet,
  MarketDataTableColumnTypes, PageState,
  SortOrder, WalletRoutes,
  WalletState
} from '../../../../constants/types'

// Actions
import { WalletActions } from '../../../../common/actions'

// Options
import { AssetFilterOptions } from '../../../../options/market-data-filter-options'
import { marketDataTableHeaders } from '../../../../options/market-data-headers'

// Components
import { SearchBar } from '../../../shared'
import { AssetsFilterDropdown, PortfolioView } from '../..'
import { MarketDataTable } from '../../../market-datatable'

// Styled Components
import {
  LoadIcon,
  LoadIconWrapper,
  StyledWrapper,
  TopRow
} from './style'

// Hooks
import { useSwap } from '../../../../common/hooks'

// Utils
import { searchCoinMarkets, sortCoinMarkets, filterCoinMarkets } from '../../../../utils/coin-market-utils'
import { makeNetworkAsset } from '../../../../options/asset-options'
import { WalletPageActions } from '../../../../page/actions'

const defaultCurrency = 'usd'
const assetsRequestLimit = 250
const displayCount = 10

interface ParamsType {
  id?: string
}

interface Props {
  toggleNav: () => void
  onClickAddAccount: (tabId: AddAccountNavTypes) => () => void
  onShowVisibleAssetsModal: (showModal: boolean) => void
  showVisibleAssetsModal: boolean
}

export const MarketView = (props: Props) => {
  const { toggleNav, onClickAddAccount, onShowVisibleAssetsModal, showVisibleAssetsModal } = props
  // State
  const [tableHeaders, setTableHeaders] = React.useState(marketDataTableHeaders)
  const [currentFilter, setCurrentFilter] = React.useState<AssetFilterOption>('all')
  const [sortOrder, setSortOrder] = React.useState<SortOrder>('desc')
  const [sortByColumnId, setSortByColumnId] = React.useState<MarketDataTableColumnTypes>('marketCap')
  const [searchTerm, setSearchTerm] = React.useState('')
  const [coinsDisplayCount, setCoinsDisplayCount] = React.useState(displayCount)
  const [selectedCoinMarket, setSelectedCoinMarket] = React.useState<BraveWallet.CoinMarket>()
  const [isSupportedInBraveWallet, setIsSupportedInBraveWallet] = React.useState<boolean>(false)
  const { id } = useParams<ParamsType>()

  // Redux
  const dispatch = useDispatch()
  const {
    isLoadingCoinMarketData,
    coinMarketData: allCoins,
    selectedNetwork,
    fullTokenList
  } = useSelector(({ wallet }: { wallet: WalletState }) => wallet)

  const {
    selectedTimeline,
    selectedAsset
  } = useSelector(({ page }: { page: PageState }) => page)

  // Hooks
  const history = useHistory()

  // Custom Hooks
  const swap = useSwap()
  const { swapAssetOptions: tradableAssets } = swap

  // Memos
  const visibleCoinMarkets = React.useMemo(() => {
    let searchResults: BraveWallet.CoinMarket[] = allCoins

    if (searchTerm !== '') {
      searchResults = searchCoinMarkets(allCoins, searchTerm)
    }
    const filteredCoins = filterCoinMarkets(searchResults, tradableAssets, currentFilter)
    const sorted = sortCoinMarkets(filteredCoins, sortOrder, sortByColumnId)
    return sorted.slice(0, coinsDisplayCount)
  }, [allCoins, sortOrder, sortByColumnId, coinsDisplayCount, searchTerm, currentFilter])

  const onSelectFilter = (value: AssetFilterOption) => {
    setCurrentFilter(value)
  }

  const onSort = React.useCallback((columnId: MarketDataTableColumnTypes, newSortOrder: SortOrder) => {
    const updatedTableHeaders = tableHeaders.map(header => {
      if (header.id === columnId) {
        return {
          ...header,
          sortOrder: newSortOrder
        }
      } else {
        return {
          ...header,
          sortOrder: undefined
        }
      }
    })

    setTableHeaders(updatedTableHeaders)
    setSortByColumnId(columnId)
    setSortOrder(newSortOrder)
  }, [])

  const onShowMoreCoins = React.useCallback(() => {
    setCoinsDisplayCount(currentCount => currentCount + displayCount)
  }, [])

  const onSelectCoinMarket = (coinMarket: BraveWallet.CoinMarket) => {
    const nativeAsset = makeNetworkAsset(selectedNetwork)
    const supportedAsset = [...fullTokenList, nativeAsset].find(a => a.symbol.toLowerCase() === coinMarket.symbol.toLowerCase())
    if (supportedAsset) {
      dispatch(WalletPageActions.selectAsset({ asset: supportedAsset, timeFrame: selectedTimeline }))
      setIsSupportedInBraveWallet(true)
    } else {
      const asset = new BraveWallet.BlockchainToken()
      asset.coingeckoId = coinMarket.id
      asset.name = coinMarket.name
      asset.contractAddress = ''
      asset.symbol = coinMarket.symbol.toUpperCase()
      asset.logo = coinMarket.image
      dispatch(WalletPageActions.selectAsset({ asset, timeFrame: selectedTimeline }))
      setIsSupportedInBraveWallet(false)
    }

    setSelectedCoinMarket(coinMarket)
    history.push(`${WalletRoutes.Market}/${coinMarket.symbol}`)
  }

  const onGoBack = () => {
    setSelectedCoinMarket(undefined)
    history.push(WalletRoutes.Market)
  }

  React.useEffect(() => {
    if (allCoins.length === 0) {
      dispatch(WalletActions.getCoinMarkets({
        vsAsset: defaultCurrency,
        limit: assetsRequestLimit
      }))
    }
  }, [allCoins])

  return (
    <StyledWrapper>
      {id && selectedAsset && selectedCoinMarket
        ? <PortfolioView
            toggleNav={toggleNav}
            onClickAddAccount={onClickAddAccount}
            onShowVisibleAssetsModal={onShowVisibleAssetsModal}
            showVisibleAssetsModal={showVisibleAssetsModal}
            isSupportedInBraveWallet={isSupportedInBraveWallet}
            hideNetworkDescription={true}
            onGoBack={onGoBack}
            selectedCoinMarket={selectedCoinMarket}
          />
        : <>
            <TopRow>
              <AssetsFilterDropdown
                options={AssetFilterOptions}
                value={currentFilter}
                onSelectFilter={onSelectFilter}
              />
              <SearchBar
                placeholder="Search"
                autoFocus={true}
                action={event => {
                  setSearchTerm(event.target.value)
                }}
                disabled={isLoadingCoinMarketData}
              />
            </TopRow>
            {isLoadingCoinMarketData
              ? <LoadIconWrapper>
                <LoadIcon />
              </LoadIconWrapper>
              : <MarketDataTable
                headers={tableHeaders}
                coinMarketData={visibleCoinMarkets}
                moreDataAvailable={visibleCoinMarkets.length > 0}
                showEmptyState={searchTerm !== '' || currentFilter !== 'all'}
                onShowMoreCoins={onShowMoreCoins}
                onSort={onSort}
                onSelectCoinMarket={onSelectCoinMarket}
              />
            }
          </>
      }
    </StyledWrapper>
  )
}
