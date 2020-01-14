/*****************************************************************************
 * Copyright (c) 2014-2019 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "../common.h"
#include "Research.h"

enum class ExpenditureType : int32_t
{
    RideConstruction = 0,
    RideRunningCosts,
    LandPurchase,
    Landscaping,
    ParkEntranceTickets,
    ParkRideTickets,
    ShopSales,
    ShopStock,
    FoodDrinkSales,
    FoodDrinkStock,
    Wages,
    Marketing,
    Research,
    Interest,
    Count
};

#define EXPENDITURE_TABLE_MONTH_COUNT 16
#define FINANCE_GRAPH_SIZE 128

extern const money32 research_cost_table[RESEARCH_FUNDING_COUNT];

extern money32 gInitialCash;
extern money32 gCash;
extern money32 gBankLoan;
extern uint8_t gBankLoanInterestRate;
extern money32 gMaxBankLoan;
extern money32 gCurrentExpenditure;
extern money32 gCurrentProfit;

/**
 * The total profit for the entire scenario that precedes
 * the current financial table.
 */
extern money32 gHistoricalProfit;

extern money32 gWeeklyProfitAverageDividend;
extern uint16_t gWeeklyProfitAverageDivisor;
extern money32 gCashHistory[FINANCE_GRAPH_SIZE];
extern money32 gWeeklyProfitHistory[FINANCE_GRAPH_SIZE];
extern money32 gParkValueHistory[FINANCE_GRAPH_SIZE];
extern money32 gExpenditureTable[EXPENDITURE_TABLE_MONTH_COUNT][static_cast<int32_t>(ExpenditureType::Count)];

bool finance_check_money_required(uint32_t flags);
bool finance_check_affordability(money32 cost, uint32_t flags);
void finance_payment(money32 amount, ExpenditureType type);
void finance_pay_wages();
void finance_pay_research();
void finance_pay_interest();
void finance_pay_ride_upkeep();
void finance_reset_history();
void finance_init();
void finance_update_daily_profit();
void finance_shift_expenditure_table();
void finance_reset_cash_to_initial();

money32 finance_get_initial_cash();
money32 finance_get_current_loan();
money32 finance_get_maximum_loan();
money32 finance_get_current_cash();

money32 finance_get_last_month_shop_profit();
