#define NO_UEFI

#include "contract_testing.h"
#include <cstddef>
#include <initializer_list>
#include <type_traits>
#include <vector>

static_assert(std::is_same_v<decltype(PLDT::CreateGame_output::returnCode), PLDT::EReturnCode>,
              "PulseEditor return codes must expose EReturnCode instead of uint8");
static_assert(sizeof(PLDT::TicketDigits) == 16, "TicketDigits wire size changed");
static_assert(offsetof(PLDT::TicketDigits, digits) == 0, "TicketDigits.digits wire offset changed");
static_assert(sizeof(PLDT::BuyTickets_input) == 272, "BuyTickets_input ABI size changed");
static_assert(offsetof(PLDT::BuyTickets_input, tickets) == 0, "BuyTickets_input.tickets ABI offset changed");
static_assert(offsetof(PLDT::BuyTickets_input, gameId) == 256, "BuyTickets_input.gameId ABI offset changed");
static_assert(offsetof(PLDT::BuyTickets_input, ticketCount) == 264, "BuyTickets_input.ticketCount ABI offset changed");
static_assert(sizeof(PLDT::PlayerSummary) == 48, "PlayerSummary ABI size changed");
static_assert(offsetof(PLDT::PlayerSummary, player) == 0, "PlayerSummary.player ABI offset changed");
static_assert(offsetof(PLDT::PlayerSummary, totalPayout) == 32, "PlayerSummary.totalPayout ABI offset changed");
static_assert(offsetof(PLDT::PlayerSummary, ticketCount) == 40, "PlayerSummary.ticketCount ABI offset changed");
static_assert(offsetof(PLDT::PlayerSummary, winningTicketCount) == 42, "PlayerSummary.winningTicketCount ABI offset changed");
static_assert(offsetof(PLDT::PlayerSummary, paidTicketCount) == 44, "PlayerSummary.paidTicketCount ABI offset changed");
static_assert(offsetof(PLDT::PlayerSummary, bonusQualifiedTicketCount) == 46, "PlayerSummary.bonusQualifiedTicketCount ABI offset changed");
static_assert(sizeof(PLDT::GetPlayers_input) == 32, "GetPlayers_input ABI size changed");
static_assert(offsetof(PLDT::GetPlayers_input, roundKey) == 0, "GetPlayers_input.roundKey ABI offset changed");
static_assert(offsetof(PLDT::GetPlayers_input, offset) == 16, "GetPlayers_input.offset ABI offset changed");
static_assert(offsetof(PLDT::GetPlayers_input, limit) == 24, "GetPlayers_input.limit ABI offset changed");
static_assert(offsetof(PLDT::GetPlayers_input, padding0) == 26, "GetPlayers_input.padding0 ABI offset changed");
static_assert(offsetof(PLDT::GetPlayers_input, padding1) == 30, "GetPlayers_input.padding1 ABI offset changed");
static_assert(sizeof(PLDT::GetPlayers_output) == 3088, "GetPlayers_output ABI size changed");
static_assert(offsetof(PLDT::GetPlayers_output, players) == 0, "GetPlayers_output.players ABI offset changed");
static_assert(offsetof(PLDT::GetPlayers_output, totalCount) == 3072, "GetPlayers_output.totalCount ABI offset changed");
static_assert(offsetof(PLDT::GetPlayers_output, returnedCount) == 3080, "GetPlayers_output.returnedCount ABI offset changed");
static_assert(offsetof(PLDT::GetPlayers_output, returnCode) == 3082, "GetPlayers_output.returnCode ABI offset changed");
static_assert(offsetof(PLDT::GetPlayers_output, padding0) == 3083, "GetPlayers_output.padding0 ABI offset changed");
static_assert(offsetof(PLDT::GetPlayers_output, padding1) == 3087, "GetPlayers_output.padding1 ABI offset changed");

struct PulseEditorTestAccess : PLDT
{
	using PLDT::automationGameSlot;
	using PLDT::calculatePlatformShares;
	using PLDT::findWalletAsset;
	using PLDT::hasPlatformAccrualCapacity;
	using PLDT::isSameAsset;
	using PLDT::resolveOwnedGame;
};

static_assert(PulseEditorTestAccess::automationGameSlot(2, PLDT_MAX_GAMES - 1) == 1,
              "PulseEditor automation slot calculation must wrap at game capacity");

TEST(PulseEditorHelpers, AssetEqualityChecksBothIssuanceFields)
{
	const Asset asset{id{1, 2, 3, 4}, 17};
	EXPECT_TRUE(PulseEditorTestAccess::isSameAsset(asset, Asset{id{1, 2, 3, 4}, 17}));
	EXPECT_FALSE(PulseEditorTestAccess::isSameAsset(asset, Asset{id{5, 6, 7, 8}, 17}));
	EXPECT_FALSE(PulseEditorTestAccess::isSameAsset(asset, Asset{id{1, 2, 3, 4}, 18}));
}

TEST(PulseEditorHelpers, WalletAssetLookupSkipsInactiveMatchingEntries)
{
	const Asset asset{id{1, 2, 3, 4}, 17};
	PLDT::CreatorWallet wallet{};
	PLDT::WalletAssetBalance entry{};
	entry.asset = asset;
	entry.balance = 11;
	wallet.assets.set(1, entry);
	entry.balance = 29;
	entry.isActive = true;
	wallet.assets.set(3, entry);
	uint64 index = 0;
	PLDT::WalletAssetBalance found{};

	EXPECT_TRUE(PulseEditorTestAccess::findWalletAsset(wallet, asset, index, found));
	EXPECT_EQ(index, 3);
	EXPECT_EQ(found.balance, 29);
}

TEST(PulseEditorHelpers, PlatformSharesAssignRoundingRemainderToDividends)
{
	uint64 developer1 = 0;
	uint64 developer2 = 0;
	uint64 dividend = 0;

	PulseEditorTestAccess::calculatePlatformShares(101, developer1, developer2, dividend);

	EXPECT_EQ(developer1, 25);
	EXPECT_EQ(developer2, 25);
	EXPECT_EQ(dividend, 51);
}

TEST(PulseEditorHelpers, PlatformAccrualCapacityAcceptsExactLimit)
{
	EXPECT_TRUE(PulseEditorTestAccess::hasPlatformAccrualCapacity(PLDT_MAX_TRANSFER_AMOUNT - 3, PLDT_MAX_TRANSFER_AMOUNT - 5,
	                                                              PLDT_MAX_TRANSFER_AMOUNT - 7, 3, 5, 7));
}

TEST(PulseEditorHelpers, PlatformAccrualCapacityRejectsOverflow)
{
	EXPECT_FALSE(PulseEditorTestAccess::hasPlatformAccrualCapacity(PLDT_MAX_TRANSFER_AMOUNT - 2, 0, 0, 3, 0, 0));
}

namespace
{
	constexpr uint16 PLDT_PROCEDURE_CREATE_GAME = 1;
	constexpr uint16 PLDT_PROCEDURE_CREATE_WALLET = 2;
	constexpr uint16 PLDT_PROCEDURE_DEPOSIT_WALLET_QUBIC = 6;
	constexpr uint16 PLDT_PROCEDURE_UPDATE_GAME_ECONOMICS = 5;
	constexpr uint16 PLDT_PROCEDURE_STOP_GAME = 7;
	constexpr uint16 PLDT_PROCEDURE_TRANSFER_SHARE_MANAGEMENT_RIGHTS = 11;
	constexpr uint16 PLDT_PROCEDURE_WITHDRAW_WALLET_QUBIC = 12;
	constexpr uint16 PLDT_PROCEDURE_BUY_TICKETS = 15;
	constexpr uint16 PLDT_PROCEDURE_SET_PLATFORM_CONFIG = 8;
	constexpr uint16 PLDT_FUNCTION_GET_GAME = 1;
	constexpr uint16 PLDT_FUNCTION_GET_GAME_RESULT = 2;
	constexpr uint16 PLDT_FUNCTION_GET_ROUND_RESULT = 2;
	constexpr uint16 PLDT_FUNCTION_GET_TICKET = 3;
	constexpr uint16 PLDT_FUNCTION_GET_WINNERS = 4;
	constexpr uint16 PLDT_FUNCTION_GET_PLATFORM_ACCOUNTING = 5;
	constexpr uint16 PLDT_FUNCTION_VALIDATE_DIGITS = 6;
	constexpr uint16 PLDT_FUNCTION_GET_PLAYER_TICKETS = 7;
	constexpr uint16 PLDT_FUNCTION_PREVIEW_GAME = 8;
	constexpr uint16 PLDT_FUNCTION_GET_GAMES = 9;
	constexpr uint16 PLDT_FUNCTION_GET_WALLET = 10;
	constexpr uint16 PLDT_FUNCTION_GET_PLAYERS = 11;

	const id CREATOR{1, 2, 3, 4};
	const id PLAYER{5, 6, 7, 8};
	const id SECOND_PLAYER{9, 10, 11, 12};
	const id OUTSIDER{13, 14, 15, 16};
	const id DEVELOPER1{21, 22, 23, 24};
	const id DEVELOPER2{25, 26, 27, 28};
	const id SHAREHOLDER{29, 30, 31, 32};
	const id PULSE_TEAM_OWNER = ID(_R, _O, _J, _V, _A, _E, _M, _F, _B, _X, _X, _Y, _N, _G, _A, _U, _A, _U, _I, _I, _X, _L, _B, _U, _P, _D, _H, _C, _D,
	                               _P, _E, _S, _Y, _Z, _O, _V, _W, _U, _Y, _E, _C, _B, _Q, _V, _Z, _R, _F, _T, _K, _A, _G, _S, _H, _T, _N, _A);

	Array<uint8, PLDT_DIGITS_ALIGNED> digits(const uint8 value)
	{
		Array<uint8, PLDT_DIGITS_ALIGNED> result{};
		for (uint8 index = 0; index < PLDT_MIN_CODE_LENGTH; ++index)
		{
			result.set(index, value);
		}
		return result;
	}

	Array<uint8, PLDT_DIGITS_ALIGNED> expectedWinningDigits(const m256i& digest, const uint64 gameId, const uint16 ticketCount,
	                                                        const uint8 codeLength, const uint8 maxDigit, const bit allowRepeatedDigits)
	{
		PLDT::BeginSettlement_randomData randomData{};
		randomData.prevSpectrumDigest = digest;
		randomData.gameId = gameId;
		randomData.roundNumber = 1;
		randomData.ticketCount = ticketCount;
		m256i hashResult{};
		KangarooTwelve(reinterpret_cast<const uint8*>(&randomData), sizeof(randomData), reinterpret_cast<uint8*>(&hashResult), sizeof(hashResult));
		const uint64 seed = hashResult.m256i_u64[0];
		Array<uint8, PLDT_DIGITS_ALIGNED> result{};
		bool used[PLDT_DIGIT_BUCKETS]{};
		for (uint64 index = 0; index < codeLength; ++index)
		{
			uint64 value = seed ^ (0x9e3779b97f4a7c15ULL * (index + 1));
			value ^= value >> 30;
			value *= 0xbf58476d1ce4e5b9ULL;
			value ^= value >> 27;
			value *= 0x94d049bb133111ebULL;
			value ^= value >> 31;
			uint8 candidate = static_cast<uint8>(value % (maxDigit + 1));
			uint8 attempts = 0;
			while (!allowRepeatedDigits && used[candidate] && attempts < PLDT_RANDOM_RETRY_LIMIT)
			{
				++attempts;
				value ^= value << 13;
				value ^= value >> 7;
				value ^= value << 17;
				candidate = static_cast<uint8>(value % (maxDigit + 1));
			}
			if (!allowRepeatedDigits && used[candidate])
			{
				for (uint8 fallback = 0; fallback <= maxDigit; ++fallback)
				{
					if (!used[fallback])
					{
						candidate = fallback;
						break;
					}
				}
			}
			result.set(index, candidate);
			used[candidate] = true;
		}
		return result;
	}

	Array<uint8, PLDT_DIGITS_ALIGNED> nonMatchingDigits(const Array<uint8, PLDT_DIGITS_ALIGNED>& winning, const uint8 codeLength,
	                                                    const uint8 maxDigit)
	{
		uint8 candidate = 0;
		for (; candidate <= maxDigit; ++candidate)
		{
			bool present = false;
			for (uint8 index = 0; index < codeLength; ++index)
			{
				present = present || winning.get(index) == candidate;
			}
			if (!present)
			{
				break;
			}
		}
		Array<uint8, PLDT_DIGITS_ALIGNED> result{};
		for (uint8 index = 0; index < codeLength; ++index)
		{
			result.set(index, candidate);
		}
		return result;
	}

	PLDT::CreateGame_input makeGame(const uint16 ticketLimit = 2, const uint64 seed = 100)
	{
		PLDT::CreateGame_input input{};
		input.tierWeightsBps.set(PLDT::payoutMatrixIndex(0, 0), 3000);
		input.tierWeightsBps.set(PLDT::payoutMatrixIndex(4, 0), 7000);
		input.startAt = DateAndTime(2025, 1, 3, 0, 0, 0);
		input.drawAt = DateAndTime(2025, 1, 4, 0, 0, 0);
		input.ticketPrice = 100;
		input.creatorPrizeSeed = seed;
		input.ticketLimit = ticketLimit;
		input.playerTicketLimit = ticketLimit;
		input.codeLength = PLDT_MIN_CODE_LENGTH;
		input.maxDigit = PLDT_MIN_MAX_DIGIT;
		input.allowRepeatedDigits = true;
		input.creatorFeePercent = 0;
		input.currencyMode = PLDT::ECurrencyMode::QUBIC;
		input.mode = PLDT::EGameMode::ONE_SHOT;
		return input;
	}
} // namespace

class ContractTestingPulseEditorV3 : public ContractTesting, public ::testing::Test
{
public:
	ContractTestingPulseEditorV3()
	{
		initEmptySpectrum();
		initEmptyUniverse();
		INIT_CONTRACT(PLDT);
		INIT_CONTRACT(QX);
		system.epoch = contractDescriptions[PLDT_CONTRACT_INDEX].constructionEpoch;
		callSystemProcedure(PLDT_CONTRACT_INDEX, INITIALIZE);
		callSystemProcedure(QX_CONTRACT_INDEX, INITIALIZE);
		setCalendar(2025, 1, 2);
	}

	void fund(const id& user, const sint64 amount) { increaseEnergy(user, amount > 0 ? amount : 1); }

	void setCalendar(const unsigned short year, const unsigned char month, const unsigned char day)
	{
		utcTime.Year = year;
		utcTime.Month = month;
		utcTime.Day = day;
		utcTime.Hour = 0;
		utcTime.Minute = 0;
		utcTime.Second = 0;
		utcTime.Nanosecond = 0;
		updateQpiTime();
	}

	void beginTickAt(const uint32 tick)
	{
		system.tick = tick;
		callSystemProcedure(PLDT_CONTRACT_INDEX, BEGIN_TICK);
	}

	void processFirstGameAt(const uint32 tick)
	{
		reinterpret_cast<PLDT::StateData*>(contractStates[PLDT_CONTRACT_INDEX])->automationCursor = 0;
		beginTickAt(tick);
	}

	template<typename Input, typename Output>
	Output procedure(const uint16 index, const Input& input, const id& user, const sint64 reward = 0)
	{
		if (spectrumIndex(user) < 0)
		{
			fund(user, reward);
		}
		Output output{};
		EXPECT_TRUE(invokeUserProcedure(PLDT_CONTRACT_INDEX, index, input, output, user, reward));
		return output;
	}

	template<typename Input, typename Output>
	Output function(const uint16 index, const Input& input) const
	{
		Output output{};
		callFunction(PLDT_CONTRACT_INDEX, index, input, output);
		return output;
	}

	PLDT::CreateGame_output createGame(const id& owner, const PLDT::CreateGame_input& input)
	{
		const auto previewResult = preview(input);
		const uint64 required = previewResult.initialQubicRequired + previewResult.operationFee;
		const uint64 creatorRefundableRequired = input.currencyMode == PLDT::ECurrencyMode::QUBIC ? input.creatorPrizeSeed : 0;
		auto balance = wallet(owner);
		if (!balance.found)
		{
			const uint64 totalShortfall = required > PLDT_DEFAULT_WALLET_CREATION_FEE ? required - PLDT_DEFAULT_WALLET_CREATION_FEE : 0;
			const uint64 initialDeposit = totalShortfall > creatorRefundableRequired ? totalShortfall : creatorRefundableRequired;
			EXPECT_EQ(createWallet(owner, static_cast<sint64>(PLDT_DEFAULT_WALLET_CREATION_FEE + initialDeposit)).returnCode,
			          PLDT::EReturnCode::SUCCESS);
			balance = wallet(owner);
		}
		const uint64 totalShortfall =
		    balance.serviceCredit + balance.refundableQubic < required ? required - balance.serviceCredit - balance.refundableQubic : 0;
		const uint64 refundableShortfall =
		    balance.refundableQubic < creatorRefundableRequired ? creatorRefundableRequired - balance.refundableQubic : 0;
		const uint64 missing = totalShortfall > refundableShortfall ? totalShortfall : refundableShortfall;
		if (missing > 0)
		{
			PLDT::DepositWalletQubic_input depositInput{};
			fund(owner, static_cast<sint64>(missing));
			EXPECT_EQ((procedure<PLDT::DepositWalletQubic_input, PLDT::DepositWalletQubic_output>(PLDT_PROCEDURE_DEPOSIT_WALLET_QUBIC, depositInput,
			                                                                                      owner, static_cast<sint64>(missing)))
			              .returnCode,
			          PLDT::EReturnCode::SUCCESS);
		}
		return procedure<PLDT::CreateGame_input, PLDT::CreateGame_output>(PLDT_PROCEDURE_CREATE_GAME, input, owner);
	}

	PLDT::CreateWallet_output createWallet(const id& owner, const sint64 amount = PLDT_DEFAULT_WALLET_CREATION_FEE)
	{
		PLDT::CreateWallet_input input{};
		fund(owner, amount);
		return procedure<PLDT::CreateWallet_input, PLDT::CreateWallet_output>(PLDT_PROCEDURE_CREATE_WALLET, input, owner, amount);
	}

	void depositWalletQubic(const id& owner, const uint64 amount)
	{
		PLDT::DepositWalletQubic_input input{};
		fund(owner, static_cast<sint64>(amount));
		ASSERT_EQ((procedure<PLDT::DepositWalletQubic_input, PLDT::DepositWalletQubic_output>(PLDT_PROCEDURE_DEPOSIT_WALLET_QUBIC, input, owner,
		                                                                                      static_cast<sint64>(amount)))
		              .returnCode,
		          PLDT::EReturnCode::SUCCESS);
	}

	PLDT::GetWallet_output wallet(const id& owner) const
	{
		PLDT::GetWallet_input input{};
		input.owner = owner;
		return function<PLDT::GetWallet_input, PLDT::GetWallet_output>(PLDT_FUNCTION_GET_WALLET, input);
	}

	PLDT::PreviewGame_output preview(const PLDT::CreateGame_input& configuration) const
	{
		return function<PLDT::PreviewGame_input, PLDT::PreviewGame_output>(PLDT_FUNCTION_PREVIEW_GAME, configuration);
	}

	PLDT::BuyTickets_output buy(const id& player, const uint64 gameId, const uint8 value)
	{
		PLDT::BuyTickets_input input{};
		input.gameId = gameId;
		input.ticketCount = 1;
		input.tickets.set(0, {digits(value)});
		return procedure<PLDT::BuyTickets_input, PLDT::BuyTickets_output>(PLDT_PROCEDURE_BUY_TICKETS, input, player, 100);
	}

	PLDT::BuyTickets_output buyAtPrice(const id& player, const uint64 gameId, const uint8 value, const sint64 price)
	{
		PLDT::BuyTickets_input input{};
		input.gameId = gameId;
		input.ticketCount = 1;
		input.tickets.set(0, {digits(value)});
		return procedure<PLDT::BuyTickets_input, PLDT::BuyTickets_output>(PLDT_PROCEDURE_BUY_TICKETS, input, player, price);
	}

	PLDT::BuyTickets_output buyAsset(const id& player, const uint64 gameId, const uint8 value)
	{
		PLDT::BuyTickets_input input{};
		input.gameId = gameId;
		input.ticketCount = 1;
		input.tickets.set(0, {digits(value)});
		return procedure<PLDT::BuyTickets_input, PLDT::BuyTickets_output>(PLDT_PROCEDURE_BUY_TICKETS, input, player);
	}

	PLDT::BuyTickets_output buyDigits(const id& player, const uint64 gameId, const Array<uint8, PLDT_DIGITS_ALIGNED>& values)
	{
		PLDT::BuyTickets_input input{};
		input.gameId = gameId;
		input.ticketCount = 1;
		input.tickets.set(0, {values});
		return procedure<PLDT::BuyTickets_input, PLDT::BuyTickets_output>(PLDT_PROCEDURE_BUY_TICKETS, input, player, 100);
	}

	PLDT::BuyTickets_output buyBatch(const id& player, const uint64 gameId, const std::initializer_list<uint8> values)
	{
		PLDT::BuyTickets_input input{};
		input.gameId = gameId;
		for (const auto value : values)
		{
			input.tickets.set(input.ticketCount++, {digits(value)});
		}
		return procedure<PLDT::BuyTickets_input, PLDT::BuyTickets_output>(PLDT_PROCEDURE_BUY_TICKETS, input, player,
		                                                                  static_cast<sint64>(100 * values.size()));
	}

	PLDT::StopGame_output stop(const id& owner, const uint64 gameId)
	{
		const auto balance = wallet(owner);
		if (balance.found && balance.serviceCredit + balance.refundableQubic < PLDT_DEFAULT_OPERATION_FEE)
		{
			PLDT::DepositWalletQubic_input depositInput{};
			fund(owner, PLDT_DEFAULT_OPERATION_FEE);
			EXPECT_EQ((procedure<PLDT::DepositWalletQubic_input, PLDT::DepositWalletQubic_output>(PLDT_PROCEDURE_DEPOSIT_WALLET_QUBIC, depositInput,
			                                                                                      owner, PLDT_DEFAULT_OPERATION_FEE))
			              .returnCode,
			          PLDT::EReturnCode::SUCCESS);
		}
		PLDT::StopGame_input input{};
		input.gameId = gameId;
		return procedure<PLDT::StopGame_input, PLDT::StopGame_output>(PLDT_PROCEDURE_STOP_GAME, input, owner);
	}

	PLDT::GetGame_output game(const uint64 gameId) const
	{
		PLDT::GetGame_input input{};
		input.gameId = gameId;
		return function<PLDT::GetGame_input, PLDT::GetGame_output>(PLDT_FUNCTION_GET_GAME, input);
	}

	PLDT::GetGameResult_output result(const uint64 gameId) const
	{
		PLDT::GetGameResult_input input{};
		input.roundKey.gameId = gameId;
		input.roundKey.roundNumber = 1;
		return function<PLDT::GetGameResult_input, PLDT::GetGameResult_output>(PLDT_FUNCTION_GET_GAME_RESULT, input);
	}

	PLDT::GetTicket_output ticket(const uint64 ticketIndex) const
	{
		PLDT::GetTicket_input input{};
		const auto* contractState = reinterpret_cast<const PLDT::StateData*>(contractStates[PLDT_CONTRACT_INDEX]);
		input.ticketId = contractState->tickets.get(ticketIndex).ticketId;
		return function<PLDT::GetTicket_input, PLDT::GetTicket_output>(PLDT_FUNCTION_GET_TICKET, input);
	}

	PLDT::SetPlatformConfig_output setPlatformConfig(const id& invocator, const id& owner)
	{
		PLDT::SetPlatformConfig_input input{};
		input.platformOwner = owner;
		input.developer1 = DEVELOPER1;
		input.developer2 = DEVELOPER2;
		return procedure<PLDT::SetPlatformConfig_input, PLDT::SetPlatformConfig_output>(PLDT_PROCEDURE_SET_PLATFORM_CONFIG, input, invocator);
	}

	PLDT::SetPlatformConfig_output configurePlatform(const id& owner) { return setPlatformConfig(platformAccounting().platformOwner, owner); }

	PLDT::WithdrawPlatformRevenue_output withdrawPlatformRevenue(const id& owner)
	{
		PLDT::WithdrawPlatformRevenue_input input{};
		return procedure<PLDT::WithdrawPlatformRevenue_input, PLDT::WithdrawPlatformRevenue_output>(9, input, owner);
	}

	PLDT::WithdrawAssetPlatformRevenue_output withdrawAssetPlatformRevenue(const id& owner, const Asset& asset)
	{
		PLDT::WithdrawAssetPlatformRevenue_input input{};
		input.asset = asset;
		input.ownershipManagingContractIndex = PLDT_CONTRACT_INDEX;
		input.possessionManagingContractIndex = PLDT_CONTRACT_INDEX;
		return procedure<PLDT::WithdrawAssetPlatformRevenue_input, PLDT::WithdrawAssetPlatformRevenue_output>(16, input, owner);
	}

	PLDT::TransferShareManagementRights_output releaseAssetManagement(const id& owner, const Asset& asset, const sint64 shares)
	{
		PLDT::TransferShareManagementRights_input input{};
		input.asset = asset;
		input.newManagingContractIndex = QX_CONTRACT_INDEX;
		input.numberOfShares = shares;
		return procedure<PLDT::TransferShareManagementRights_input, PLDT::TransferShareManagementRights_output>(11, input, owner, 100);
	}

	PLDT::GetPlatformAccounting_output platformAccounting() const
	{
		PLDT::GetPlatformAccounting_input input{};
		return function<PLDT::GetPlatformAccounting_input, PLDT::GetPlatformAccounting_output>(PLDT_FUNCTION_GET_PLATFORM_ACCOUNTING, input);
	}

	PLDT::GetGames_output games() const
	{
		PLDT::GetGames_input input{};
		input.limit = 64;
		return function<PLDT::GetGames_input, PLDT::GetGames_output>(PLDT_FUNCTION_GET_GAMES, input);
	}

	PLDT::ValidateDigits_output validate(const Array<uint8, PLDT_DIGITS_ALIGNED>& values, const uint8 codeLength, const uint8 maxDigit,
	                                     const bit allowRepeatedDigits) const
	{
		PLDT::ValidateDigits_input input{};
		input.digits = values;
		input.codeLength = codeLength;
		input.maxDigit = maxDigit;
		input.allowRepeatedDigits = allowRepeatedDigits;
		return function<PLDT::ValidateDigits_input, PLDT::ValidateDigits_output>(PLDT_FUNCTION_VALIDATE_DIGITS, input);
	}

	id contractId() const { return id(PLDT_CONTRACT_INDEX, 0, 0, 0); }

	sint64 issueAsset(const Asset& asset, const sint64 shares)
	{
		increaseEnergy(asset.issuer, 1000000000ULL);
		QX::IssueAsset_input input{asset.assetName, shares, 0, 0};
		QX::IssueAsset_output output{};
		EXPECT_TRUE(invokeUserProcedure(QX_CONTRACT_INDEX, 1, input, output, asset.issuer, 1000000000ULL));
		return output.issuedNumberOfShares;
	}

	sint64 transferAsset(const Asset& asset, const id& currentOwner, const id& newOwner, const sint64 shares)
	{
		increaseEnergy(currentOwner, 100);
		QX::TransferShareOwnershipAndPossession_input input{};
		input.assetName = asset.assetName;
		input.issuer = asset.issuer;
		input.newOwnerAndPossessor = newOwner;
		input.numberOfShares = shares;
		QX::TransferShareOwnershipAndPossession_output output{};
		EXPECT_TRUE(invokeUserProcedure(QX_CONTRACT_INDEX, 2, input, output, currentOwner, 100));
		return output.transferredNumberOfShares;
	}

	sint64 transferAssetManagement(const Asset& asset, const id& owner, const sint64 shares, const uint16 manager)
	{
		if (manager == PLDT_CONTRACT_INDEX && !wallet(owner).found)
		{
			EXPECT_EQ(createWallet(owner).returnCode, PLDT::EReturnCode::SUCCESS);
		}
		if (spectrumIndex(owner) < 0)
		{
			fund(owner, 1);
		}
		QX::TransferShareManagementRights_input input{};
		input.asset = asset;
		input.newManagingContractIndex = manager;
		input.numberOfShares = shares;
		QX::TransferShareManagementRights_output output{};
		EXPECT_TRUE(invokeUserProcedure(QX_CONTRACT_INDEX, 9, input, output, owner, 0));
		return output.transferredNumberOfShares;
	}

	void issuePulseEditorSharesTo(const id& owner)
	{
		std::vector<std::pair<m256i, unsigned int>> shares{{owner, NUMBER_OF_COMPUTORS}};
		issueContractShares(PLDT_CONTRACT_INDEX, shares, false);
	}

	uint16 proposeEconomics(const id& shareholder, uint64 variable, sint64 value)
	{
		ProposalDataYesNo proposal{};
		proposal.epoch = system.epoch;
		proposal.type = ProposalTypes::VariableYesNo;
		proposal.data.variableOptions.variable = variable;
		proposal.data.variableOptions.value = value;
		return procedure<ProposalDataYesNo, uint16>(65534, proposal, shareholder);
	}

	bool voteEconomics(const id& shareholder, uint16 proposalIndex, sint64 choice = 1)
	{
		ProposalMultiVoteDataV1 vote{};
		vote.proposalIndex = proposalIndex;
		vote.proposalType = ProposalTypes::VariableYesNo;
		vote.proposalTick = system.tick;
		vote.voteValues.set(0, choice);
		return procedure<ProposalMultiVoteDataV1, bit>(65535, vote, shareholder);
	}

	PLDT::GetPlayerTickets_output playerTickets(const id& player, const uint64 gameId, const uint64 offset = 0) const
	{
		PLDT::GetPlayerTickets_input input{};
		input.player = player;
		input.roundKey.gameId = gameId;
		input.roundKey.roundNumber = 1;
		input.offset = offset;
		input.limit = 256;
		return function<PLDT::GetPlayerTickets_input, PLDT::GetPlayerTickets_output>(PLDT_FUNCTION_GET_PLAYER_TICKETS, input);
	}

	PLDT::GetWinners_output winners(const uint64 gameId, const uint64 offset = 0) const
	{
		PLDT::GetWinners_input input{};
		input.roundKey.gameId = gameId;
		input.roundKey.roundNumber = 1;
		input.offset = offset;
		input.limit = 256;
		return function<PLDT::GetWinners_input, PLDT::GetWinners_output>(PLDT_FUNCTION_GET_WINNERS, input);
	}

	PLDT::GetPlayers_output players(const uint64 gameId, const uint64 roundNumber = 1, const uint64 offset = 0,
	                                const uint16 limit = PLDT_PLAYERS_PAGE_CAPACITY) const
	{
		PLDT::GetPlayers_input input{};
		input.roundKey.gameId = gameId;
		input.roundKey.roundNumber = roundNumber;
		input.offset = offset;
		input.limit = limit;
		return function<PLDT::GetPlayers_input, PLDT::GetPlayers_output>(PLDT_FUNCTION_GET_PLAYERS, input);
	}
};

TEST_F(ContractTestingPulseEditorV3, OwnedGameResolutionReturnsStoredSlotAndGame)
{
	auto* contractState = reinterpret_cast<PLDT::StateData*>(contractStates[PLDT_CONTRACT_INDEX]);
	PLDT::Game stored{};
	stored.gameId = 5;
	stored.owner = CREATOR;
	stored.status = PLDT::EGameStatus::SCHEDULED;
	contractState->games.set(5, stored);
	auto& state = *reinterpret_cast<QPI::ContractState<PLDT::StateData, PLDT_CONTRACT_INDEX>*>(contractState);
	uint16 slot = 0;
	PLDT::Game resolved{};

	EXPECT_EQ(PulseEditorTestAccess::resolveOwnedGame(state, stored.gameId, CREATOR, slot, resolved), PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(static_cast<uint32>(slot), 5U);
	EXPECT_EQ(resolved.gameId, stored.gameId);
}

TEST_F(ContractTestingPulseEditorV3, CreateWalletSeparatesServiceCreditFromRefundableQubic)
{
	constexpr sint64 extraDeposit = 250000;
	const auto created = createWallet(CREATOR, PLDT_DEFAULT_WALLET_CREATION_FEE + extraDeposit);
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);

	const auto balance = wallet(CREATOR);
	EXPECT_TRUE(balance.found);
	EXPECT_EQ(balance.serviceCredit, PLDT_DEFAULT_WALLET_CREATION_FEE);
	EXPECT_EQ(balance.refundableQubic, extraDeposit);
}

TEST_F(ContractTestingPulseEditorV3, DuplicateCreateWalletRefundsRewardAndPreservesOriginalLedger)
{
	ASSERT_EQ(createWallet(CREATOR, PLDT_DEFAULT_WALLET_CREATION_FEE + 250).returnCode, PLDT::EReturnCode::SUCCESS);
	fund(CREATOR, 77);
	const sint64 externalBalanceBefore = getBalance(CREATOR);
	PLDT::CreateWallet_input input{};

	const auto duplicate = procedure<PLDT::CreateWallet_input, PLDT::CreateWallet_output>(PLDT_PROCEDURE_CREATE_WALLET, input, CREATOR, 77);

	EXPECT_EQ(duplicate.returnCode, PLDT::EReturnCode::INVALID_STATE);
	EXPECT_EQ(getBalance(CREATOR), externalBalanceBefore);
	const auto stored = wallet(CREATOR);
	ASSERT_TRUE(stored.found);
	EXPECT_EQ(stored.serviceCredit, PLDT_DEFAULT_WALLET_CREATION_FEE);
	EXPECT_EQ(stored.refundableQubic, 250);
}

TEST_F(ContractTestingPulseEditorV3, UnderfundedCreateWalletRefundsRewardAndDoesNotAllocate)
{
	constexpr sint64 underfundedReward = PLDT_DEFAULT_WALLET_CREATION_FEE - 1;
	fund(OUTSIDER, underfundedReward);
	const sint64 externalBalanceBefore = getBalance(OUTSIDER);
	PLDT::CreateWallet_input input{};

	const auto rejected =
	    procedure<PLDT::CreateWallet_input, PLDT::CreateWallet_output>(PLDT_PROCEDURE_CREATE_WALLET, input, OUTSIDER, underfundedReward);

	EXPECT_EQ(rejected.returnCode, PLDT::EReturnCode::INSUFFICIENT_FUNDS);
	EXPECT_EQ(getBalance(OUTSIDER), externalBalanceBefore);
	EXPECT_FALSE(wallet(OUTSIDER).found);
	EXPECT_EQ(static_cast<uint32>(platformAccounting().walletCount), 0U);
}

TEST_F(ContractTestingPulseEditorV3, WalletCapacityAccepts1024thAndRefunds1025th)
{
	PLDT::CreateWallet_input input{};
	for (uint64 index = 0; index < PLDT_MAX_WALLETS; ++index)
	{
		const id owner{10000 + index, 20000 + index, 30000 + index, 40000 + index};
		fund(owner, PLDT_DEFAULT_WALLET_CREATION_FEE);
		const auto created = procedure<PLDT::CreateWallet_input, PLDT::CreateWallet_output>(PLDT_PROCEDURE_CREATE_WALLET, input, owner,
		                                                                                    PLDT_DEFAULT_WALLET_CREATION_FEE);
		ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS) << "wallet index " << index;
	}
	ASSERT_EQ(static_cast<uint32>(platformAccounting().walletCount), 1024U);

	const id overflowOwner{50000, 50001, 50002, 50003};
	constexpr sint64 overflowReward = PLDT_DEFAULT_WALLET_CREATION_FEE + 31;
	fund(overflowOwner, overflowReward);
	const sint64 externalBalanceBefore = getBalance(overflowOwner);
	const auto rejected =
	    procedure<PLDT::CreateWallet_input, PLDT::CreateWallet_output>(PLDT_PROCEDURE_CREATE_WALLET, input, overflowOwner, overflowReward);

	EXPECT_EQ(rejected.returnCode, PLDT::EReturnCode::STORAGE_FULL);
	EXPECT_EQ(getBalance(overflowOwner), externalBalanceBefore);
	EXPECT_FALSE(wallet(overflowOwner).found);
	EXPECT_EQ(static_cast<uint32>(platformAccounting().walletCount), 1024U);
}

TEST_F(ContractTestingPulseEditorV3, PreviewRejectsEconomicallyTrivialCodeSpace)
{
	auto configuration = makeGame();
	configuration.tierWeightsBps = {};
	configuration.tierWeightsBps.set(PLDT::payoutMatrixIndex(0, 0), PLDT_TIER_BPS_SCALE);
	configuration.codeLength = 1;
	configuration.maxDigit = 0;
	configuration.allowRepeatedDigits = true;

	EXPECT_EQ(preview(configuration).returnCode, PLDT::EReturnCode::INVALID_VALUE);
}

TEST_F(ContractTestingPulseEditorV3, InvalidCreatorMutationStillConsumesOperationFee)
{
	ASSERT_EQ(createWallet(CREATOR).returnCode, PLDT::EReturnCode::SUCCESS);
	const auto before = wallet(CREATOR);
	auto configuration = makeGame();
	configuration.codeLength = 0;
	setContractFeeReserve(PLDT_CONTRACT_INDEX, 1000);

	const auto created = procedure<PLDT::CreateGame_input, PLDT::CreateGame_output>(PLDT_PROCEDURE_CREATE_GAME, configuration, CREATOR);

	EXPECT_EQ(created.returnCode, PLDT::EReturnCode::INVALID_VALUE);
	EXPECT_EQ(wallet(CREATOR).serviceCredit, before.serviceCredit - 1000);
	EXPECT_EQ(getContractFeeReserve(PLDT_CONTRACT_INDEX), 2000);
}

TEST_F(ContractTestingPulseEditorV3, ServiceCreditCannotFundQubicPrize)
{
	ASSERT_EQ(createWallet(CREATOR).returnCode, PLDT::EReturnCode::SUCCESS);
	auto configuration = makeGame(1, 100);

	const auto created = procedure<PLDT::CreateGame_input, PLDT::CreateGame_output>(PLDT_PROCEDURE_CREATE_GAME, configuration, CREATOR);

	EXPECT_EQ(created.returnCode, PLDT::EReturnCode::INSUFFICIENT_FUNDS);
}

TEST_F(ContractTestingPulseEditorV3, UnusedServiceCreditUnlocksAfterLastGameCloses)
{
	ASSERT_EQ(createWallet(CREATOR).returnCode, PLDT::EReturnCode::SUCCESS);
	auto configuration = makeGame(1, 0);
	const auto created = procedure<PLDT::CreateGame_input, PLDT::CreateGame_output>(PLDT_PROCEDURE_CREATE_GAME, configuration, CREATOR);
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
	ASSERT_EQ(stop(CREATOR, created.gameId).returnCode, PLDT::EReturnCode::SUCCESS);
	processFirstGameAt(PLDT_TICK_UPDATE_PERIOD);
	ASSERT_EQ(game(created.gameId).returnCode, PLDT::EReturnCode::INVALID_GAME);

	const auto available = wallet(CREATOR).serviceCredit;
	ASSERT_GT(available, 0ULL);
	PLDT::WithdrawWalletQubic_input withdrawInput{};
	withdrawInput.amount = available;
	const auto withdrawn =
	    procedure<PLDT::WithdrawWalletQubic_input, PLDT::WithdrawWalletQubic_output>(PLDT_PROCEDURE_WITHDRAW_WALLET_QUBIC, withdrawInput, CREATOR);

	EXPECT_EQ(withdrawn.returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(withdrawn.amountPaid, available);
}

TEST_F(ContractTestingPulseEditorV3, AutomationVisitsAdjacentGameSlots)
{
	auto* contractState = reinterpret_cast<PLDT::StateData*>(contractStates[PLDT_CONTRACT_INDEX]);
	PLDT::Game scheduled{};
	scheduled.startAt = DateAndTime(2025, 1, 3, 0, 0, 0);
	scheduled.drawAt = DateAndTime(2025, 1, 4, 0, 0, 0);
	scheduled.status = PLDT::EGameStatus::SCHEDULED;
	scheduled.gameId = 1024;
	contractState->games.set(0, scheduled);
	scheduled.gameId = 1025;
	contractState->games.set(1, scheduled);
	setCalendar(2025, 1, 3);

	beginTickAt(PLDT_TICK_UPDATE_PERIOD);

	EXPECT_EQ(contractState->games.get(0).status, PLDT::EGameStatus::SELLING);
	EXPECT_EQ(contractState->games.get(1).status, PLDT::EGameStatus::SELLING);
}

TEST_F(ContractTestingPulseEditorV3, AutomationSkipsEmptySlotsAcrossWrapAndPreservesTickPeriod)
{
	auto* contractState = reinterpret_cast<PLDT::StateData*>(contractStates[PLDT_CONTRACT_INDEX]);
	PLDT::Game scheduled{};
	scheduled.startAt = DateAndTime(2025, 1, 3, 0, 0, 0);
	scheduled.drawAt = DateAndTime(2025, 1, 5, 0, 0, 0);
	scheduled.status = PLDT::EGameStatus::SCHEDULED;
	scheduled.gameId = 1924;
	contractState->games.set(900, scheduled);
	scheduled.gameId = 1024;
	contractState->games.set(0, scheduled);
	contractState->activeGameCount = 2;
	contractState->automationCursor = 800;
	setCalendar(2025, 1, 3);

	beginTickAt(99);
	EXPECT_EQ(contractState->games.get(900).status, PLDT::EGameStatus::SCHEDULED);
	EXPECT_EQ(contractState->games.get(0).status, PLDT::EGameStatus::SCHEDULED);
	EXPECT_EQ(static_cast<uint32>(contractState->automationCursor), 800);

	beginTickAt(100);
	EXPECT_EQ(contractState->games.get(900).status, PLDT::EGameStatus::SELLING);
	EXPECT_EQ(contractState->games.get(0).status, PLDT::EGameStatus::SELLING);
	EXPECT_EQ(static_cast<uint32>(contractState->automationCursor), 800);
}

TEST_F(ContractTestingPulseEditorV3, AutomationLimitsNonemptyGamesAndContinuesAfterLastVisitedSlot)
{
	auto* contractState = reinterpret_cast<PLDT::StateData*>(contractStates[PLDT_CONTRACT_INDEX]);
	PLDT::Game scheduled{};
	scheduled.startAt = DateAndTime(2025, 1, 3, 0, 0, 0);
	scheduled.drawAt = DateAndTime(2025, 1, 4, 0, 0, 0);
	scheduled.status = PLDT::EGameStatus::SCHEDULED;
	for (uint16 i = 0; i < 33; ++i)
	{
		scheduled.gameId = 1024 + i * 20;
		contractState->games.set(i * 20, scheduled);
	}
	contractState->activeGameCount = 33;
	setCalendar(2025, 1, 3);

	beginTickAt(100);
	for (uint16 i = 0; i < 32; ++i)
	{
		EXPECT_EQ(contractState->games.get(i * 20).status, PLDT::EGameStatus::SELLING);
	}
	EXPECT_EQ(contractState->games.get(640).status, PLDT::EGameStatus::SCHEDULED);
	EXPECT_EQ(static_cast<uint32>(contractState->automationCursor), 621);

	beginTickAt(200);
	EXPECT_EQ(contractState->games.get(640).status, PLDT::EGameStatus::SELLING);
	// Already-selling games still consume visits; the second pass ends at slot 600.
	EXPECT_EQ(static_cast<uint32>(contractState->automationCursor), 601);
}

TEST_F(ContractTestingPulseEditorV3, AutomationFullRegistryAdvancesByThirtyTwoGames)
{
	auto* contractState = reinterpret_cast<PLDT::StateData*>(contractStates[PLDT_CONTRACT_INDEX]);
	PLDT::Game scheduled{};
	scheduled.startAt = DateAndTime(2025, 1, 3, 0, 0, 0);
	scheduled.drawAt = DateAndTime(2025, 1, 4, 0, 0, 0);
	scheduled.status = PLDT::EGameStatus::SCHEDULED;
	for (uint16 i = 0; i < PLDT_MAX_GAMES; ++i)
	{
		scheduled.gameId = 1024 + i;
		contractState->games.set(i, scheduled);
	}
	contractState->activeGameCount = PLDT_MAX_GAMES;
	setCalendar(2025, 1, 3);
	beginTickAt(100);
	EXPECT_EQ(static_cast<uint32>(contractState->automationCursor), 32);
	EXPECT_EQ(contractState->games.get(31).status, PLDT::EGameStatus::SELLING);
	EXPECT_EQ(contractState->games.get(32).status, PLDT::EGameStatus::SCHEDULED);
	beginTickAt(200);
	EXPECT_EQ(static_cast<uint32>(contractState->automationCursor), 64);
	for (uint16 i = 0; i < PLDT_MAX_GAMES; ++i)
	{
		EXPECT_EQ(contractState->games.get(i).status, i < 64 ? PLDT::EGameStatus::SELLING : PLDT::EGameStatus::SCHEDULED);
	}
}

TEST_F(ContractTestingPulseEditorV3, AutomationCountsGamesThatCloseDuringProcessing)
{
	ASSERT_EQ(createWallet(CREATOR).returnCode, PLDT::EReturnCode::SUCCESS);
	ASSERT_EQ(createWallet(SECOND_PLAYER).returnCode, PLDT::EReturnCode::SUCCESS);
	ASSERT_EQ(createWallet(OUTSIDER).returnCode, PLDT::EReturnCode::SUCCESS);
	auto* contractState = reinterpret_cast<PLDT::StateData*>(contractStates[PLDT_CONTRACT_INDEX]);
	PLDT::CreatorWallet creatorWallet{};
	for (const auto& owner : {CREATOR, SECOND_PLAYER, OUTSIDER})
	{
		ASSERT_TRUE(contractState->wallets.get(owner, creatorWallet));
		creatorWallet.activeGameCount = 11;
		contractState->wallets.replace(owner, creatorWallet);
	}
	PLDT::Game closing{};
	closing.startAt = DateAndTime(2025, 1, 3, 0, 0, 0);
	closing.drawAt = DateAndTime(2025, 1, 4, 0, 0, 0);
	closing.status = PLDT::EGameStatus::SELLING;
	closing.mode = PLDT::EGameMode::ONE_SHOT;
	closing.roundNumber = 1;
	for (uint16 i = 0; i < 33; ++i)
	{
		closing.owner = i % 3 == 0 ? CREATOR : (i % 3 == 1 ? SECOND_PLAYER : OUTSIDER);
		closing.gameId = 1024 + i * 2;
		contractState->games.set(i * 2, closing);
	}
	contractState->activeGameCount = 33;
	setCalendar(2025, 1, 4);

	beginTickAt(100);
	EXPECT_EQ(contractState->resultCounter, 32ULL);
	EXPECT_EQ(static_cast<uint32>(contractState->activeGameCount), 1);
	EXPECT_EQ(contractState->games.get(64).status, PLDT::EGameStatus::SELLING);
	EXPECT_EQ(static_cast<uint32>(contractState->automationCursor), 63);
	beginTickAt(200);
	EXPECT_EQ(contractState->resultCounter, 33ULL);
	EXPECT_EQ(static_cast<uint32>(contractState->activeGameCount), 0);
}

TEST_F(ContractTestingPulseEditorV3, AutomationRevisitsSparsePermanentGameOnNextPassWithoutDoubleFee)
{
	auto configuration = makeGame(2, 100);
	configuration.mode = PLDT::EGameMode::PERMANENT;
	const auto created = createGame(CREATOR, configuration);
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
	const auto serviceBefore = wallet(CREATOR).serviceCredit;
	setCalendar(2025, 1, 4);
	beginTickAt(100);
	EXPECT_EQ(game(created.gameId).game.roundNumber, 2ULL);
	EXPECT_EQ(wallet(CREATOR).serviceCredit, serviceBefore - 10000);
	EXPECT_EQ(platformAccounting().resultCounter, 1ULL);

	setCalendar(2025, 1, 5);
	beginTickAt(200);
	EXPECT_EQ(game(created.gameId).game.roundNumber, 3ULL);
	EXPECT_EQ(wallet(CREATOR).serviceCredit, serviceBefore - 20000);
	EXPECT_EQ(platformAccounting().resultCounter, 2ULL);
}

TEST_F(ContractTestingPulseEditorV3, AutomationEmptyRegistryStillReclaimsTicketsAndExpiresWallets)
{
	const auto created = createGame(CREATOR, makeGame(1, 0));
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
	setCalendar(2025, 1, 3);
	ASSERT_EQ(buy(PLAYER, created.gameId, 0).returnCode, PLDT::EReturnCode::SUCCESS);
	beginTickAt(100);
	auto* contractState = reinterpret_cast<PLDT::StateData*>(contractStates[PLDT_CONTRACT_INDEX]);
	ASSERT_EQ(static_cast<uint32>(contractState->activeGameCount), 0);
	ASSERT_EQ(contractState->freeTicketCount, 0U);
	contractState->nextUnusedTicketSlot = PLDT_MAX_TICKETS;
	contractState->automationCursor = 777;
	const auto walletIndex = contractState->wallets.getElementIndex(CREATOR);
	ASSERT_GE(walletIndex, 0);
	contractState->walletAutomationCursor = static_cast<uint16>(walletIndex);
	system.epoch = static_cast<uint16>(system.epoch + 2);
	const auto creatorWallet = wallet(CREATOR);
	const auto balanceBefore = getBalance(CREATOR);

	beginTickAt(200);
	EXPECT_EQ(static_cast<uint32>(contractState->automationCursor), 777);
	EXPECT_EQ(contractState->freeTicketCount, 1U);
	EXPECT_FALSE(result(created.gameId).roundResult.detailsAvailable);
	EXPECT_FALSE(wallet(CREATOR).found);
	EXPECT_EQ(getBalance(CREATOR), balanceBefore + creatorWallet.serviceCredit + creatorWallet.refundableQubic);
}

TEST_F(ContractTestingPulseEditorV3, AutomationCountsRetryableFailuresAgainstGameLimit)
{
	std::vector<PLDT::CreateGame_output> created;
	for (uint16 i = 0; i < 33; ++i)
	{
		const id owner = i < 16 ? CREATOR : (i < 32 ? SECOND_PLAYER : OUTSIDER);
		created.push_back(createGame(owner, makeGame(2, 100)));
		ASSERT_EQ(created.back().returnCode, PLDT::EReturnCode::SUCCESS);
	}
	auto* contractState = reinterpret_cast<PLDT::StateData*>(contractStates[PLDT_CONTRACT_INDEX]);
	// Simulate exhausted wallet credit capacity: returning each seed must remain retryable.
	for (const auto& owner : {CREATOR, SECOND_PLAYER, OUTSIDER})
	{
		PLDT::CreatorWallet fullWallet{};
		ASSERT_TRUE(contractState->wallets.get(owner, fullWallet));
		fullWallet.refundableQubic = PLDT_MAX_TRANSFER_AMOUNT;
		contractState->wallets.replace(owner, fullWallet);
	}
	setCalendar(2025, 1, 4);
	beginTickAt(100);
	for (uint16 i = 0; i < 32; ++i)
	{
		const auto& pending = contractState->games.get(created[i].slot);
		EXPECT_EQ(pending.status, PLDT::EGameStatus::FINALIZING);
		EXPECT_EQ(pending.pendingPrizePoolPayout, 100ULL);
	}
	EXPECT_EQ(contractState->games.get(created[32].slot).status, PLDT::EGameStatus::SCHEDULED);
	EXPECT_EQ(contractState->resultCounter, 0ULL);
	EXPECT_EQ(static_cast<uint32>(contractState->automationCursor), 32U);

	// The next pass must reach the deferred game despite the preceding retry failures.
	beginTickAt(200);
	EXPECT_EQ(contractState->games.get(created[32].slot).status, PLDT::EGameStatus::FINALIZING);
	EXPECT_EQ(contractState->games.get(created[32].slot).pendingPrizePoolPayout, 100ULL);
	EXPECT_EQ(contractState->resultCounter, 0ULL);
}

TEST_F(ContractTestingPulseEditorV3, AutomationReclamationUsesOnlyBudgetRemainingAfterSettlement)
{
	auto configuration = makeGame(65, 0);
	configuration.tierWeightsBps = {};
	configuration.tierWeightsBps.set(PLDT::payoutMatrixIndex(4, 0), 10000);
	const auto oldGame = createGame(CREATOR, configuration);
	ASSERT_EQ(oldGame.returnCode, PLDT::EReturnCode::SUCCESS);
	const m256i digest(25, 26, 27, 28);
	etalonTick.prevSpectrumDigest = digest;
	const auto oldWinning =
	    expectedWinningDigits(digest, oldGame.gameId, 65, configuration.codeLength, configuration.maxDigit, configuration.allowRepeatedDigits);
	setCalendar(2025, 1, 3);
	fund(PLAYER, 6500);
	for (uint16 i = 0; i < 65; ++i)
	{
		ASSERT_EQ(buyDigits(PLAYER, oldGame.gameId, oldWinning).returnCode, PLDT::EReturnCode::SUCCESS);
	}
	beginTickAt(100);
	beginTickAt(200);
	beginTickAt(300);
	ASSERT_EQ(result(oldGame.gameId).returnCode, PLDT::EReturnCode::SUCCESS);

	configuration.ticketLimit = 1;
	configuration.playerTicketLimit = 1;
	configuration.startAt = DateAndTime(2025, 1, 4, 0, 0, 0);
	configuration.drawAt = DateAndTime(2025, 1, 5, 0, 0, 0);
	const auto currentGame = createGame(CREATOR, configuration);
	ASSERT_EQ(currentGame.returnCode, PLDT::EReturnCode::SUCCESS);
	const auto currentWinning =
	    expectedWinningDigits(digest, currentGame.gameId, 1, configuration.codeLength, configuration.maxDigit, configuration.allowRepeatedDigits);
	setCalendar(2025, 1, 4);
	const auto purchase = buyDigits(PLAYER, currentGame.gameId, currentWinning);
	ASSERT_EQ(purchase.returnCode, PLDT::EReturnCode::SUCCESS);
	auto* contractState = reinterpret_cast<PLDT::StateData*>(contractStates[PLDT_CONTRACT_INDEX]);
	ASSERT_EQ(contractState->freeTicketCount, 0U);
	// Trigger reclamation pressure without filling a million live ticket slots.
	contractState->nextUnusedTicketSlot = PLDT_MAX_TICKETS;

	beginTickAt(400);
	ASSERT_EQ(result(currentGame.gameId).returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(ticket(purchase.ticketIndexes.get(0)).ticket.status, PLDT::ETicketStatus::PAID);
	// One classification and one payment leave exactly 62 of the 64 actions for reclamation.
	EXPECT_EQ(contractState->freeTicketCount, 62U);
	EXPECT_FALSE(result(oldGame.gameId).roundResult.detailsAvailable);
	EXPECT_TRUE(result(currentGame.gameId).roundResult.detailsAvailable);

	beginTickAt(500);
	EXPECT_EQ(contractState->freeTicketCount, 66U);
	EXPECT_FALSE(result(currentGame.gameId).roundResult.detailsAvailable);
}

TEST_F(ContractTestingPulseEditorV3, AutomationBusyPermanentGameCannotStarveFollowingSettlement)
{
	auto configuration = makeGame(32, 0);
	configuration.mode = PLDT::EGameMode::PERMANENT;
	configuration.tierWeightsBps = {};
	// Tickets containing only zero have no misplaced matches; every possible exact count wins.
	for (uint8 exact = 0; exact <= 4; ++exact)
	{
		configuration.tierWeightsBps.set(PLDT::payoutMatrixIndex(exact, 0), 2000);
	}
	const auto busy = createGame(CREATOR, configuration);
	ASSERT_EQ(busy.returnCode, PLDT::EReturnCode::SUCCESS);
	configuration.mode = PLDT::EGameMode::ONE_SHOT;
	configuration.ticketLimit = 1;
	configuration.playerTicketLimit = 1;
	const auto waiting = createGame(OUTSIDER, configuration);
	ASSERT_EQ(waiting.returnCode, PLDT::EReturnCode::SUCCESS);
	setCalendar(2025, 1, 3);
	fund(PLAYER, 20000);
	ASSERT_EQ(buy(PLAYER, waiting.gameId, 0).returnCode, PLDT::EReturnCode::SUCCESS);

	for (uint32 tick = 100; tick <= 400; tick += 100)
	{
		if (game(busy.gameId).game.ticketCount == 0)
		{
			for (uint16 batch = 0; batch < 2; ++batch)
			{
				PLDT::BuyTickets_input purchase{};
				purchase.gameId = busy.gameId;
				purchase.ticketCount = 16;
				ASSERT_EQ((procedure<PLDT::BuyTickets_input, PLDT::BuyTickets_output>(PLDT_PROCEDURE_BUY_TICKETS, purchase, PLAYER, 1600)).returnCode,
				          PLDT::EReturnCode::SUCCESS);
			}
		}
		beginTickAt(tick);
	}
	const auto completed = result(waiting.gameId);
	ASSERT_EQ(completed.returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(completed.roundResult.totalPaid, 97ULL);
}

TEST_F(ContractTestingPulseEditorV3, WalletQubicDepositAndWithdrawalPreserveServiceCredit)
{
	ASSERT_EQ(createWallet(CREATOR).returnCode, PLDT::EReturnCode::SUCCESS);
	PLDT::DepositWalletQubic_input depositInput{};
	fund(CREATOR, 500);
	const auto deposited =
	    procedure<PLDT::DepositWalletQubic_input, PLDT::DepositWalletQubic_output>(PLDT_PROCEDURE_DEPOSIT_WALLET_QUBIC, depositInput, CREATOR, 500);
	ASSERT_EQ(deposited.returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(deposited.refundableQubic, 500);

	PLDT::WithdrawWalletQubic_input withdrawInput{};
	withdrawInput.amount = 200;
	const auto withdrawn =
	    procedure<PLDT::WithdrawWalletQubic_input, PLDT::WithdrawWalletQubic_output>(PLDT_PROCEDURE_WITHDRAW_WALLET_QUBIC, withdrawInput, CREATOR);
	ASSERT_EQ(withdrawn.returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(withdrawn.amountPaid, 200);
	const auto balance = wallet(CREATOR);
	EXPECT_EQ(balance.serviceCredit, PLDT_DEFAULT_WALLET_CREATION_FEE);
	EXPECT_EQ(balance.refundableQubic, 300);
}

TEST_F(ContractTestingPulseEditorV3, CreateGameConsumesWalletServiceCreditWithoutInvocationReward)
{
	ASSERT_EQ(createWallet(CREATOR, PLDT_DEFAULT_WALLET_CREATION_FEE + 100).returnCode, PLDT::EReturnCode::SUCCESS);
	const auto input = makeGame(1, 100);
	const auto created = procedure<PLDT::CreateGame_input, PLDT::CreateGame_output>(PLDT_PROCEDURE_CREATE_GAME, input, CREATOR);
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
	const auto balance = wallet(CREATOR);
	EXPECT_EQ(balance.serviceCredit, PLDT_DEFAULT_WALLET_CREATION_FEE - PLDT_DEFAULT_OPERATION_FEE - PLDT_DEFAULT_ROUND_FEE);
	EXPECT_EQ(balance.refundableQubic, 0);
	EXPECT_EQ(static_cast<uint32>(balance.activeGameCount), 1U);
}

TEST_F(ContractTestingPulseEditorV3, IdleWalletExpiresAfterOneCompleteEpoch)
{
	ASSERT_EQ(createWallet(CREATOR, PLDT_DEFAULT_WALLET_CREATION_FEE + 500).returnCode, PLDT::EReturnCode::SUCCESS);
	auto* contractState = reinterpret_cast<PLDT::StateData*>(contractStates[PLDT_CONTRACT_INDEX]);
	const auto walletIndex = contractState->wallets.getElementIndex(CREATOR);
	ASSERT_GE(walletIndex, 0);
	contractState->walletAutomationCursor = static_cast<uint16>(walletIndex);
	const auto balanceBeforeExpiry = getBalance(CREATOR);
	system.epoch = static_cast<uint16>(system.epoch + 2);
	beginTickAt(PLDT_TICK_UPDATE_PERIOD);

	EXPECT_FALSE(wallet(CREATOR).found);
	EXPECT_EQ(getBalance(CREATOR), balanceBeforeExpiry + 500);
	const auto accounting = platformAccounting();
	EXPECT_EQ(accounting.developer1Accrued, PLDT_DEFAULT_WALLET_CREATION_FEE / 4);
	EXPECT_EQ(accounting.developer2Accrued, PLDT_DEFAULT_WALLET_CREATION_FEE / 4);
	EXPECT_EQ(accounting.dividendAccrued, PLDT_DEFAULT_WALLET_CREATION_FEE / 2);
}

TEST_F(ContractTestingPulseEditorV3, RefundableQubicExpiryTransferFailureRetriesWithoutDoubleAccrual)
{
	ASSERT_EQ(createWallet(CREATOR, PLDT_DEFAULT_WALLET_CREATION_FEE + 500).returnCode, PLDT::EReturnCode::SUCCESS);
	auto* contractState = reinterpret_cast<PLDT::StateData*>(contractStates[PLDT_CONTRACT_INDEX]);
	const auto walletIndex = contractState->wallets.getElementIndex(CREATOR);
	ASSERT_GE(walletIndex, 0);
	contractState->walletAutomationCursor = static_cast<uint16>(walletIndex);
	system.epoch = static_cast<uint16>(system.epoch + 2);
	const auto selfIndex = spectrumIndex(contractId());
	ASSERT_GE(selfIndex, 0);
	ASSERT_TRUE(decreaseEnergy(selfIndex, getBalance(contractId())));

	beginTickAt(PLDT_TICK_UPDATE_PERIOD);

	const auto pending = wallet(CREATOR);
	ASSERT_TRUE(pending.found);
	EXPECT_EQ(pending.status, PLDT::EWalletStatus::EXPIRING);
	EXPECT_EQ(pending.serviceCredit, PLDT_DEFAULT_WALLET_CREATION_FEE);
	EXPECT_EQ(pending.refundableQubic, 500);
	const auto accountingAfterFailure = platformAccounting();
	EXPECT_EQ(accountingAfterFailure.developer1Accrued, 0);
	EXPECT_EQ(accountingAfterFailure.developer2Accrued, 0);
	EXPECT_EQ(accountingAfterFailure.dividendAccrued, 0);

	increaseEnergy(contractId(), 500);
	contractState->walletAutomationCursor = static_cast<uint16>(walletIndex);
	const sint64 creatorBalanceBeforeRetry = getBalance(CREATOR);
	beginTickAt(PLDT_TICK_UPDATE_PERIOD * 2);

	EXPECT_FALSE(wallet(CREATOR).found);
	EXPECT_EQ(getBalance(CREATOR), creatorBalanceBeforeRetry + 500);
	const auto accountingAfterRetry = platformAccounting();
	EXPECT_EQ(accountingAfterRetry.developer1Accrued, 250000);
	EXPECT_EQ(accountingAfterRetry.developer2Accrued, 250000);
	EXPECT_EQ(accountingAfterRetry.dividendAccrued, 500000);
}

TEST_F(ContractTestingPulseEditorV3, WalletAutomationCleansAccumulatedHashMapTombstones)
{
	auto* contractState = reinterpret_cast<PLDT::StateData*>(contractStates[PLDT_CONTRACT_INDEX]);
	PLDT::CreatorWallet storedWallet{};
	for (uint64 i = 0; i <= PLDT_WALLET_MAP_CAPACITY / 2; ++i)
	{
		const id owner{i + 1000, 0, 0, 0};
		ASSERT_GE(contractState->wallets.set(owner, storedWallet), 0);
		ASSERT_GE(contractState->wallets.removeByKey(owner), 0);
	}
	ASSERT_TRUE(contractState->wallets.needsCleanup());

	beginTickAt(PLDT_TICK_UPDATE_PERIOD);

	EXPECT_FALSE(contractState->wallets.needsCleanup());
}

TEST_F(ContractTestingPulseEditorV3, FailedExpiryReleaseLeavesUserOwnedSharesAvailableForManualRelease)
{
	ASSERT_EQ(createWallet(CREATOR).returnCode, PLDT::EReturnCode::SUCCESS);
	const Asset asset{CREATOR, assetNameFromString("PEDRET")};
	ASSERT_EQ(issueAsset(asset, 10), 10);
	ASSERT_EQ(transferAssetManagement(asset, CREATOR, 10, PLDT_CONTRACT_INDEX), 10);

	auto* contractState = reinterpret_cast<PLDT::StateData*>(contractStates[PLDT_CONTRACT_INDEX]);
	PLDT::CreatorWallet storedWallet{};
	ASSERT_TRUE(contractState->wallets.get(CREATOR, storedWallet));
	storedWallet.serviceCredit = 0;
	ASSERT_TRUE(contractState->wallets.replace(CREATOR, storedWallet));
	const auto walletIndex = contractState->wallets.getElementIndex(CREATOR);
	ASSERT_GE(walletIndex, 0);
	contractState->walletAutomationCursor = static_cast<uint16>(walletIndex);
	system.epoch = static_cast<uint16>(system.epoch + 2);

	beginTickAt(PLDT_TICK_UPDATE_PERIOD);

	EXPECT_FALSE(wallet(CREATOR).found);
	EXPECT_EQ(numberOfPossessedShares(asset.assetName, asset.issuer, CREATOR, CREATOR, PLDT_CONTRACT_INDEX, PLDT_CONTRACT_INDEX), 10);
	fund(CREATOR, 100);
	const auto released = releaseAssetManagement(CREATOR, asset, 10);
	EXPECT_EQ(released.returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(numberOfPossessedShares(asset.assetName, asset.issuer, CREATOR, CREATOR, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX), 10);
}

TEST_F(ContractTestingPulseEditorV3, IncomingManagementRightsCreditExistingWallet)
{
	ASSERT_EQ(createWallet(CREATOR).returnCode, PLDT::EReturnCode::SUCCESS);
	const Asset asset{CREATOR, assetNameFromString("PEDWLT")};
	ASSERT_EQ(issueAsset(asset, 10), 10);
	ASSERT_EQ(transferAssetManagement(asset, CREATOR, 10, PLDT_CONTRACT_INDEX), 10);

	const auto balance = wallet(CREATOR);
	ASSERT_TRUE(balance.found);
	ASSERT_EQ(balance.assetCount, 1);
	EXPECT_EQ(balance.assets.get(0).asset.assetName, asset.assetName);
	EXPECT_EQ(balance.assets.get(0).asset.issuer, asset.issuer);
	EXPECT_EQ(balance.assets.get(0).balance, 10);
}

TEST_F(ContractTestingPulseEditorV3, WalletAcceptsSixteenthManagedAssetAndRejectsSeventeenth)
{
	ASSERT_EQ(createWallet(CREATOR).returnCode, PLDT::EReturnCode::SUCCESS);
	const char* assetNames[17] = {"PEA0001", "PEA0002", "PEA0003", "PEA0004", "PEA0005", "PEA0006", "PEA0007", "PEA0008", "PEA0009",
	                              "PEA0010", "PEA0011", "PEA0012", "PEA0013", "PEA0014", "PEA0015", "PEA0016", "PEA0017"};
	Asset seventeenth{};
	for (uint8 index = 0; index < 17; ++index)
	{
		const Asset asset{CREATOR, assetNameFromString(assetNames[index])};
		ASSERT_EQ(issueAsset(asset, 1), 1) << "asset index " << static_cast<uint32>(index);
		const sint64 transferred = transferAssetManagement(asset, CREATOR, 1, PLDT_CONTRACT_INDEX);
		if (index < PLDT_MAX_WALLET_ASSETS)
		{
			ASSERT_EQ(transferred, 1) << "asset index " << static_cast<uint32>(index);
		}
		else
		{
			seventeenth = asset;
			EXPECT_EQ(transferred, 0);
		}
	}

	EXPECT_EQ(static_cast<uint32>(wallet(CREATOR).assetCount), 16U);
	EXPECT_EQ(numberOfPossessedShares(seventeenth.assetName, seventeenth.issuer, CREATOR, CREATOR, PLDT_CONTRACT_INDEX, PLDT_CONTRACT_INDEX), 0);
	EXPECT_EQ(numberOfPossessedShares(seventeenth.assetName, seventeenth.issuer, CREATOR, CREATOR, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX), 1);
}

TEST_F(ContractTestingPulseEditorV3, IncomingAssetCreditAcceptsExactMaxAndRejectsOverflow)
{
	ASSERT_EQ(createWallet(CREATOR).returnCode, PLDT::EReturnCode::SUCCESS);
	const Asset asset{CREATOR, assetNameFromString("PEAMAX")};
	ASSERT_EQ(issueAsset(asset, 3), 3);
	ASSERT_EQ(transferAssetManagement(asset, CREATOR, 1, PLDT_CONTRACT_INDEX), 1);
	auto* contractState = reinterpret_cast<PLDT::StateData*>(contractStates[PLDT_CONTRACT_INDEX]);
	PLDT::CreatorWallet storedWallet{};
	ASSERT_TRUE(contractState->wallets.get(CREATOR, storedWallet));
	auto storedAsset = storedWallet.assets.get(0);
	storedAsset.balance = PLDT_MAX_TRANSFER_AMOUNT - 1;
	storedWallet.assets.set(0, storedAsset);
	ASSERT_TRUE(contractState->wallets.replace(CREATOR, storedWallet));

	EXPECT_EQ(transferAssetManagement(asset, CREATOR, 1, PLDT_CONTRACT_INDEX), 1);
	EXPECT_EQ(wallet(CREATOR).assets.get(0).balance, PLDT_MAX_TRANSFER_AMOUNT);
	EXPECT_EQ(transferAssetManagement(asset, CREATOR, 1, PLDT_CONTRACT_INDEX), 0);
	EXPECT_EQ(wallet(CREATOR).assets.get(0).balance, PLDT_MAX_TRANSFER_AMOUNT);
	EXPECT_EQ(numberOfPossessedShares(asset.assetName, asset.issuer, CREATOR, CREATOR, PLDT_CONTRACT_INDEX, PLDT_CONTRACT_INDEX), 2);
	EXPECT_EQ(numberOfPossessedShares(asset.assetName, asset.issuer, CREATOR, CREATOR, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX), 1);
}

TEST_F(ContractTestingPulseEditorV3, FailedManualAssetReleasePreservesWalletLedgerAndCustody)
{
	ASSERT_EQ(createWallet(CREATOR).returnCode, PLDT::EReturnCode::SUCCESS);
	const Asset asset{CREATOR, assetNameFromString("PEARLS")};
	ASSERT_EQ(issueAsset(asset, 10), 10);
	ASSERT_EQ(transferAssetManagement(asset, CREATOR, 10, PLDT_CONTRACT_INDEX), 10);
	PLDT::TransferShareManagementRights_input input{};
	input.asset = asset;
	input.newManagingContractIndex = QX_CONTRACT_INDEX;
	input.numberOfShares = 10;

	const auto failed = procedure<PLDT::TransferShareManagementRights_input, PLDT::TransferShareManagementRights_output>(
	    PLDT_PROCEDURE_TRANSFER_SHARE_MANAGEMENT_RIGHTS, input, CREATOR);

	EXPECT_EQ(failed.returnCode, PLDT::EReturnCode::TRANSFER_FAILED);
	EXPECT_EQ(failed.transferResult, -100);
	EXPECT_EQ(wallet(CREATOR).assets.get(0).balance, 10);
	EXPECT_EQ(numberOfPossessedShares(asset.assetName, asset.issuer, CREATOR, CREATOR, PLDT_CONTRACT_INDEX, PLDT_CONTRACT_INDEX), 10);
}

TEST_F(ContractTestingPulseEditorV3, FailedInitialAssetTransferPreservesWalletAndGameState)
{
	ASSERT_EQ(createWallet(CREATOR).returnCode, PLDT::EReturnCode::SUCCESS);
	const Asset asset{CREATOR, assetNameFromString("PEATRF")};
	ASSERT_EQ(issueAsset(asset, 10), 10);
	ASSERT_EQ(transferAssetManagement(asset, CREATOR, 10, PLDT_CONTRACT_INDEX), 10);
	auto* contractState = reinterpret_cast<PLDT::StateData*>(contractStates[PLDT_CONTRACT_INDEX]);
	PLDT::CreatorWallet storedWallet{};
	ASSERT_TRUE(contractState->wallets.get(CREATOR, storedWallet));
	auto storedAsset = storedWallet.assets.get(0);
	storedAsset.balance = 100;
	storedWallet.assets.set(0, storedAsset);
	ASSERT_TRUE(contractState->wallets.replace(CREATOR, storedWallet));
	auto configuration = makeGame(1, 100);
	configuration.currencyMode = PLDT::ECurrencyMode::ASSET;
	configuration.currencyAsset = asset;
	configuration.ownershipManagingContractIndex = PLDT_CONTRACT_INDEX;
	configuration.possessionManagingContractIndex = PLDT_CONTRACT_INDEX;
	const auto walletBefore = wallet(CREATOR);

	const auto rejected = procedure<PLDT::CreateGame_input, PLDT::CreateGame_output>(PLDT_PROCEDURE_CREATE_GAME, configuration, CREATOR);

	EXPECT_EQ(rejected.returnCode, PLDT::EReturnCode::TRANSFER_FAILED);
	const auto walletAfter = wallet(CREATOR);
	EXPECT_EQ(walletAfter.serviceCredit, walletBefore.serviceCredit - 1000);
	EXPECT_EQ(walletAfter.assets.get(0).balance, 100);
	EXPECT_EQ(static_cast<uint32>(walletAfter.activeGameCount), 0U);
	EXPECT_EQ(static_cast<uint32>(platformAccounting().activeGameCount), 0U);
	EXPECT_EQ(numberOfPossessedShares(asset.assetName, asset.issuer, CREATOR, CREATOR, PLDT_CONTRACT_INDEX, PLDT_CONTRACT_INDEX), 10);
}

TEST_F(ContractTestingPulseEditorV3, PreviewRejectsInvalidTierWeightTotalAndDates)
{
	auto invalidWeights = makeGame();
	invalidWeights.tierWeightsBps.set(PLDT::payoutMatrixIndex(1, 0), 6999);
	PLDT::PreviewGame_input previewInput{};
	previewInput = invalidWeights;
	const auto weights = function<PLDT::PreviewGame_input, PLDT::PreviewGame_output>(PLDT_FUNCTION_PREVIEW_GAME, previewInput);
	EXPECT_EQ(weights.returnCode, PLDT::EReturnCode::INVALID_VALUE);

	auto invalidDates = makeGame();
	invalidDates.drawAt = invalidDates.startAt;
	previewInput = invalidDates;
	const auto dates = function<PLDT::PreviewGame_input, PLDT::PreviewGame_output>(PLDT_FUNCTION_PREVIEW_GAME, previewInput);
	EXPECT_EQ(dates.returnCode, PLDT::EReturnCode::INVALID_VALUE);
}

TEST_F(ContractTestingPulseEditorV3, PreviewRejectsUnknownCurrencyModeAndEmptyBonusAsset)
{
	auto configuration = makeGame();
	configuration.currencyMode = static_cast<PLDT::ECurrencyMode>(2);
	EXPECT_EQ(preview(configuration).returnCode, PLDT::EReturnCode::INVALID_VALUE);

	configuration = makeGame();
	configuration.bonusAssetCount = 1;
	configuration.bonusMultiplierBps = 12000;
	EXPECT_EQ(preview(configuration).returnCode, PLDT::EReturnCode::INVALID_VALUE);

	configuration.bonusAssets.set(0, Asset{CREATOR, assetNameFromString("MISSING")});
	EXPECT_EQ(preview(configuration).returnCode, PLDT::EReturnCode::INVALID_VALUE);
}

TEST_F(ContractTestingPulseEditorV3, AtomicCreateStoresScheduledImmutableGame)
{
	fund(CREATOR, 10100);
	const sint64 balanceBefore = getBalance(CREATOR);
	const auto created = createGame(CREATOR, makeGame());
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_NE(created.gameId, 0);
	EXPECT_EQ(getBalance(CREATOR), balanceBefore);
	EXPECT_EQ(wallet(CREATOR).serviceCredit, PLDT_DEFAULT_WALLET_CREATION_FEE - 11000);

	const auto stored = game(created.gameId);
	ASSERT_EQ(stored.returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(stored.game.status, PLDT::EGameStatus::SCHEDULED);
	EXPECT_EQ(stored.game.prizePool, 100);
	EXPECT_EQ(stored.game.startAt, DateAndTime(2025, 1, 3, 0, 0, 0));
}

TEST_F(ContractTestingPulseEditorV3, CreateRejectsWrongQubicRewardAndInsufficientAssetSeedWithoutPersisting)
{
	fund(CREATOR, 199);
	const sint64 balanceBefore = getBalance(CREATOR);
	const auto configuration = makeGame(1, 100);
	const auto wrongReward = procedure<PLDT::CreateGame_input, PLDT::CreateGame_output>(PLDT_PROCEDURE_CREATE_GAME, configuration, CREATOR, 99);
	EXPECT_EQ(wrongReward.returnCode, PLDT::EReturnCode::TICKET_INVALID_PRICE);
	EXPECT_EQ(getBalance(CREATOR), balanceBefore);
	EXPECT_EQ(static_cast<uint32>(games().totalActive), 0U);

	const Asset currency{CREATOR, assetNameFromString("PEDSEED")};
	ASSERT_EQ(issueAsset(currency, 100), 100);
	ASSERT_EQ(transferAssetManagement(currency, CREATOR, 100, PLDT_CONTRACT_INDEX), 100);
	auto assetConfiguration = makeGame(1, 101);
	assetConfiguration.currencyMode = PLDT::ECurrencyMode::ASSET;
	assetConfiguration.currencyAsset = currency;
	assetConfiguration.ownershipManagingContractIndex = PLDT_CONTRACT_INDEX;
	assetConfiguration.possessionManagingContractIndex = PLDT_CONTRACT_INDEX;
	const auto insufficient = createGame(CREATOR, assetConfiguration);
	EXPECT_EQ(insufficient.returnCode, PLDT::EReturnCode::INSUFFICIENT_FUNDS);
	EXPECT_EQ(static_cast<uint32>(games().totalActive), 0U);
}

TEST_F(ContractTestingPulseEditorV3, CreationRejectsDrawBeyondTheSchedulingHorizon)
{
	auto configuration = makeGame(1, 0);
	configuration.startAt = DateAndTime(2026, 1, 4, 0, 0, 0);
	configuration.drawAt = DateAndTime(2026, 1, 5, 0, 0, 0);
	ASSERT_EQ(preview(configuration).returnCode, PLDT::EReturnCode::INVALID_VALUE);

	const auto created = createGame(CREATOR, configuration);

	EXPECT_EQ(created.returnCode, PLDT::EReturnCode::INVALID_VALUE);
	EXPECT_EQ(static_cast<uint32>(games().totalActive), 0U);
}

TEST_F(ContractTestingPulseEditorV3, OneShotCreationChargesDefaultNonRefundableRoundFee)
{
	auto configuration = makeGame(1, 100);
	const auto created = createGame(CREATOR, configuration);

	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(game(created.gameId).game.prizePool, 100ULL);
}

TEST_F(ContractTestingPulseEditorV3, CreationRejectsPlatformLiabilityOverflowBeforeChargingTheRound)
{
	auto* contractState = reinterpret_cast<PLDT::StateData*>(contractStates[PLDT_CONTRACT_INDEX]);
	contractState->developer1Accrued = PLDT_MAX_TRANSFER_AMOUNT;
	const auto created = createGame(CREATOR, makeGame(1, 100));
	EXPECT_EQ(created.returnCode, PLDT::EReturnCode::STORAGE_FULL);
	EXPECT_EQ(static_cast<uint32>(platformAccounting().activeGameCount), 0U);
	EXPECT_EQ(wallet(CREATOR).serviceCredit + wallet(CREATOR).refundableQubic, PLDT_DEFAULT_WALLET_CREATION_FEE - 900);
}

TEST_F(ContractTestingPulseEditorV3, TicketPurchaseRejectsPlatformLiabilityOverflowBeforeCustody)
{
	auto configuration = makeGame(1, 0);
	configuration.ticketPrice = 10000;
	const auto created = createGame(CREATOR, configuration);
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
	auto* contractState = reinterpret_cast<PLDT::StateData*>(contractStates[PLDT_CONTRACT_INDEX]);
	contractState->developer1Accrued = PLDT_MAX_TRANSFER_AMOUNT;
	setCalendar(2025, 1, 3);
	const auto purchased = buyAtPrice(PLAYER, created.gameId, 0, 10000);
	EXPECT_EQ(purchased.returnCode, PLDT::EReturnCode::STORAGE_FULL);
	EXPECT_EQ(static_cast<uint32>(game(created.gameId).game.ticketCount), 0U);
	EXPECT_EQ(getBalance(PLAYER), 10000);
}

TEST_F(ContractTestingPulseEditorV3, SharedWalletReturnedSeedFundsNextRoundWithoutGameTopUp)
{
	auto input = makeGame(2, 100);
	input.mode = PLDT::EGameMode::PERMANENT;
	const auto created = createGame(CREATOR, input);
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
	ASSERT_EQ(wallet(CREATOR).refundableQubic, 0ULL);

	setCalendar(2025, 1, 4);
	processFirstGameAt(100);

	const auto current = game(created.gameId);
	ASSERT_EQ(current.returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(current.game.roundNumber, 2ULL);
	EXPECT_EQ(current.game.prizePool, 100ULL);
	EXPECT_EQ(wallet(CREATOR).refundableQubic, 0ULL);
	EXPECT_EQ(wallet(CREATOR).serviceCredit, 979000ULL);
	EXPECT_EQ(result(created.gameId).gameResult.gameStopReason, PLDT::EGameStopReason::NONE);
	processFirstGameAt(200);
	EXPECT_EQ(wallet(CREATOR).serviceCredit, 979000ULL);
	EXPECT_EQ(platformAccounting().resultCounter, 1ULL);
}

TEST_F(ContractTestingPulseEditorV3, AssetAccountingCapacityRejectionRefundsUnexpectedQubicForOneAndTwoTicketBatches)
{
	const Asset currency{CREATOR, assetNameFromString("PEDREF")};
	ASSERT_EQ(issueAsset(currency, 400), 400);
	ASSERT_EQ(transferAsset(currency, CREATOR, PLAYER, 300), 300);
	ASSERT_EQ(transferAssetManagement(currency, CREATOR, 100, PLDT_CONTRACT_INDEX), 100);
	ASSERT_EQ(transferAssetManagement(currency, PLAYER, 300, PLDT_CONTRACT_INDEX), 300);
	auto input = makeGame(4, 100);
	input.currencyMode = PLDT::ECurrencyMode::ASSET;
	input.currencyAsset = currency;
	input.ownershipManagingContractIndex = PLDT_CONTRACT_INDEX;
	input.possessionManagingContractIndex = PLDT_CONTRACT_INDEX;
	const auto created = createGame(CREATOR, input);
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
	auto* contractState = reinterpret_cast<PLDT::StateData*>(contractStates[PLDT_CONTRACT_INDEX]);
	const auto accountingSlot = game(created.gameId).game.assetAccountingLink - 1;
	auto accounting = contractState->assetAccounting.get(accountingSlot);
	accounting.dividendAccrued = PLDT_MAX_TRANSFER_AMOUNT;
	contractState->assetAccounting.set(accountingSlot, accounting);
	setCalendar(2025, 1, 3);
	fund(PLAYER, 1000);
	const auto before = getBalance(PLAYER);
	PLDT::BuyTickets_input single{};
	single.gameId = created.gameId;
	single.ticketCount = 1;
	single.tickets.set(0, {digits(0)});
	const auto rejectedSingle = procedure<PLDT::BuyTickets_input, PLDT::BuyTickets_output>(PLDT_PROCEDURE_BUY_TICKETS, single, PLAYER, 77);
	EXPECT_EQ(rejectedSingle.returnCode, PLDT::EReturnCode::STORAGE_FULL);
	EXPECT_EQ(getBalance(PLAYER), before);
	PLDT::BuyTickets_input batch{};
	batch.gameId = created.gameId;
	batch.ticketCount = 2;
	batch.tickets.set(0, {digits(0)});
	batch.tickets.set(1, {digits(1)});
	const auto rejectedBatch = procedure<PLDT::BuyTickets_input, PLDT::BuyTickets_output>(PLDT_PROCEDURE_BUY_TICKETS, batch, PLAYER, 123);
	EXPECT_EQ(rejectedBatch.returnCode, PLDT::EReturnCode::STORAGE_FULL);
	EXPECT_EQ(getBalance(PLAYER), before);
	EXPECT_EQ(static_cast<uint32>(game(created.gameId).game.ticketCount), 0U);
	EXPECT_EQ(game(created.gameId).game.prizePool, 100ULL);
	EXPECT_EQ(wallet(PLAYER).assets.get(0).balance, 300ULL);
	EXPECT_EQ(numberOfPossessedShares(currency.assetName, currency.issuer, PLAYER, PLAYER, PLDT_CONTRACT_INDEX, PLDT_CONTRACT_INDEX), 300);
}

TEST_F(ContractTestingPulseEditorV3, SharedWalletZeroSeedStillPaysEachRoundFee)
{
	auto input = makeGame(2, 0);
	input.mode = PLDT::EGameMode::PERMANENT;
	const auto created = createGame(CREATOR, input);
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
	setCalendar(2025, 1, 4);
	processFirstGameAt(100);
	ASSERT_EQ(game(created.gameId).returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(game(created.gameId).game.roundNumber, 2ULL);
	EXPECT_EQ(game(created.gameId).game.prizePool, 0ULL);
	EXPECT_EQ(wallet(CREATOR).serviceCredit, 979000ULL);
}

TEST_F(ContractTestingPulseEditorV3, SharedWalletFeeUsesServiceCreditBeforeRefundableQubic)
{
	reinterpret_cast<PLDT::StateData*>(contractStates[PLDT_CONTRACT_INDEX])->walletCreationFee = 5100;
	ASSERT_EQ(createWallet(CREATOR, 15200).returnCode, PLDT::EReturnCode::SUCCESS);
	const auto created = procedure<PLDT::CreateGame_input, PLDT::CreateGame_output>(PLDT_PROCEDURE_CREATE_GAME, makeGame(2, 100), CREATOR);
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(wallet(CREATOR).serviceCredit, 0ULL);
	EXPECT_EQ(wallet(CREATOR).refundableQubic, 4100ULL);
	EXPECT_EQ(game(created.gameId).game.prizePool, 100ULL);
}

TEST_F(ContractTestingPulseEditorV3, SharedWalletInsufficientCreationFeeDoesNotReserveSeed)
{
	reinterpret_cast<PLDT::StateData*>(contractStates[PLDT_CONTRACT_INDEX])->walletCreationFee = 1000;
	ASSERT_EQ(createWallet(CREATOR, 11099).returnCode, PLDT::EReturnCode::SUCCESS);
	const auto before = platformAccounting();
	const auto created = procedure<PLDT::CreateGame_input, PLDT::CreateGame_output>(PLDT_PROCEDURE_CREATE_GAME, makeGame(2, 100), CREATOR);
	EXPECT_EQ(created.returnCode, PLDT::EReturnCode::INSUFFICIENT_FUNDS);
	EXPECT_EQ(wallet(CREATOR).serviceCredit, 0ULL);
	EXPECT_EQ(wallet(CREATOR).refundableQubic, 10099ULL);
	EXPECT_EQ(static_cast<uint32>(wallet(CREATOR).activeGameCount), 0U);
	EXPECT_EQ(platformAccounting().developer1Accrued, before.developer1Accrued);
	EXPECT_EQ(platformAccounting().resultCounter, 0ULL);
}

TEST_F(ContractTestingPulseEditorV3, SharedWalletTwoGamesCannotSpendTheSameRoundFee)
{
	reinterpret_cast<PLDT::StateData*>(contractStates[PLDT_CONTRACT_INDEX])->walletCreationFee = 32000;
	ASSERT_EQ(createWallet(CREATOR, 32000).returnCode, PLDT::EReturnCode::SUCCESS);
	auto input = makeGame(2, 0);
	input.mode = PLDT::EGameMode::PERMANENT;
	const auto first = createGame(CREATOR, input);
	const auto second = createGame(CREATOR, input);
	ASSERT_EQ(first.returnCode, PLDT::EReturnCode::SUCCESS);
	ASSERT_EQ(second.returnCode, PLDT::EReturnCode::SUCCESS);
	ASSERT_EQ(wallet(CREATOR).serviceCredit, 10000ULL);
	setCalendar(2025, 1, 4);
	processFirstGameAt(100);
	EXPECT_EQ(game(first.gameId).game.roundNumber, 2ULL);
	EXPECT_EQ(game(second.gameId).returnCode, PLDT::EReturnCode::INVALID_GAME);
	EXPECT_EQ(result(second.gameId).gameResult.gameStopReason, PLDT::EGameStopReason::OUT_OF_FUNDS);
	EXPECT_EQ(wallet(CREATOR).serviceCredit, 0ULL);
	EXPECT_EQ(static_cast<uint32>(wallet(CREATOR).activeGameCount), 1U);
	EXPECT_EQ(platformAccounting().developer1Accrued, 7500ULL);
	EXPECT_EQ(platformAccounting().developer2Accrued, 7500ULL);
	EXPECT_EQ(platformAccounting().dividendAccrued, 15000ULL);
	processFirstGameAt(200);
	EXPECT_EQ(platformAccounting().resultCounter, 2ULL);
}

TEST_F(ContractTestingPulseEditorV3, SharedWalletWithdrawalAffectsNextSeedButNotCurrentWinner)
{
	ASSERT_EQ(createWallet(CREATOR, PLDT_DEFAULT_WALLET_CREATION_FEE + 200).returnCode, PLDT::EReturnCode::SUCCESS);
	auto input = makeGame(1, 100);
	input.mode = PLDT::EGameMode::PERMANENT;
	const auto created = createGame(CREATOR, input);
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
	PLDT::WithdrawWalletQubic_input withdrawal{};
	withdrawal.amount = 100;
	ASSERT_EQ(
	    (procedure<PLDT::WithdrawWalletQubic_input, PLDT::WithdrawWalletQubic_output>(PLDT_PROCEDURE_WITHDRAW_WALLET_QUBIC, withdrawal, CREATOR))
	        .returnCode,
	    PLDT::EReturnCode::SUCCESS);
	const m256i digest(9, 10, 11, 12);
	etalonTick.prevSpectrumDigest = digest;
	const auto winning = expectedWinningDigits(digest, created.gameId, 1, input.codeLength, input.maxDigit, input.allowRepeatedDigits);
	setCalendar(2025, 1, 3);
	const auto purchase = buyDigits(PLAYER, created.gameId, winning);
	ASSERT_EQ(purchase.returnCode, PLDT::EReturnCode::SUCCESS);
	processFirstGameAt(100);
	EXPECT_EQ(ticket(purchase.ticketIndexes.get(0)).ticket.payout, 197ULL);
	EXPECT_EQ(game(created.gameId).returnCode, PLDT::EReturnCode::INVALID_GAME);
	EXPECT_EQ(result(created.gameId).gameResult.gameStopReason, PLDT::EGameStopReason::OUT_OF_FUNDS);
	EXPECT_EQ(wallet(CREATOR).serviceCredit, 989000ULL);
	EXPECT_EQ(wallet(CREATOR).refundableQubic, 0ULL);
	EXPECT_EQ(platformAccounting().developer1Accrued, 2500ULL);
}

TEST_F(ContractTestingPulseEditorV3, SharedWalletCreatorRevenueFundsNextSeedAfterWinnerPayout)
{
	auto input = makeGame(1, 9);
	input.mode = PLDT::EGameMode::PERMANENT;
	input.creatorFeePercent = 10;
	const auto created = createGame(CREATOR, input);
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
	const m256i digest(9, 10, 11, 12);
	etalonTick.prevSpectrumDigest = digest;
	const auto winning = expectedWinningDigits(digest, created.gameId, 1, input.codeLength, input.maxDigit, input.allowRepeatedDigits);
	setCalendar(2025, 1, 3);
	const auto purchase = buyDigits(PLAYER, created.gameId, winning);
	ASSERT_EQ(purchase.returnCode, PLDT::EReturnCode::SUCCESS);
	ASSERT_EQ(wallet(CREATOR).refundableQubic, 0ULL);
	processFirstGameAt(100);
	ASSERT_EQ(game(created.gameId).returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(game(created.gameId).game.roundNumber, 2ULL);
	EXPECT_EQ(game(created.gameId).game.prizePool, 9ULL);
	EXPECT_EQ(ticket(purchase.ticketIndexes.get(0)).ticket.payout, 97ULL);
	EXPECT_EQ(wallet(CREATOR).refundableQubic, 0ULL);
	EXPECT_EQ(wallet(CREATOR).serviceCredit, 979000ULL);
}

TEST_F(ContractTestingPulseEditorV3, SharedWalletPlatformCapacityRetryDoesNotChargeTwice)
{
	auto input = makeGame(2, 100);
	input.mode = PLDT::EGameMode::PERMANENT;
	const auto created = createGame(CREATOR, input);
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
	auto* contractState = reinterpret_cast<PLDT::StateData*>(contractStates[PLDT_CONTRACT_INDEX]);
	contractState->developer1Accrued = PLDT_MAX_TRANSFER_AMOUNT;
	setCalendar(2025, 1, 4);
	processFirstGameAt(100);
	ASSERT_EQ(game(created.gameId).game.status, PLDT::EGameStatus::FINALIZING);
	EXPECT_EQ(wallet(CREATOR).refundableQubic, 100ULL);
	EXPECT_EQ(wallet(CREATOR).serviceCredit, 989000ULL);
	processFirstGameAt(200);
	EXPECT_EQ(wallet(CREATOR).refundableQubic, 100ULL);
	EXPECT_EQ(wallet(CREATOR).serviceCredit, 989000ULL);
	EXPECT_EQ(platformAccounting().resultCounter, 0ULL);
	contractState->developer1Accrued = 0;
	processFirstGameAt(300);
	EXPECT_EQ(game(created.gameId).game.roundNumber, 2ULL);
	EXPECT_EQ(wallet(CREATOR).serviceCredit, 979000ULL);
	EXPECT_EQ(wallet(CREATOR).refundableQubic, 0ULL);
	EXPECT_EQ(platformAccounting().developer1Accrued, 2500ULL);
	EXPECT_EQ(platformAccounting().resultCounter, 1ULL);
	processFirstGameAt(400);
	EXPECT_EQ(wallet(CREATOR).serviceCredit, 979000ULL);
}

TEST_F(ContractTestingPulseEditorV3, SharedWalletAssetRolloverRetriesCustodyWithoutChargingOrLosingUpdate)
{
	const Asset currency{CREATOR, assetNameFromString("PEDNEXT")};
	ASSERT_EQ(issueAsset(currency, 150), 150);
	ASSERT_EQ(transferAssetManagement(currency, CREATOR, 150, PLDT_CONTRACT_INDEX), 150);
	auto input = makeGame(2, 100);
	input.mode = PLDT::EGameMode::PERMANENT;
	input.currencyMode = PLDT::ECurrencyMode::ASSET;
	input.currencyAsset = currency;
	input.ownershipManagingContractIndex = PLDT_CONTRACT_INDEX;
	input.possessionManagingContractIndex = PLDT_CONTRACT_INDEX;
	const auto created = createGame(CREATOR, input);
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
	PLDT::UpdateGameEconomics_input update{};
	update.gameId = created.gameId;
	update.ticketPrice = 100;
	update.creatorPrizeSeed = 150;
	update.ticketLimit = 2;
	update.playerTicketLimit = 2;
	ASSERT_EQ((procedure<PLDT::UpdateGameEconomics_input, PLDT::UpdateGameEconomics_output>(PLDT_PROCEDURE_UPDATE_GAME_ECONOMICS, update, CREATOR))
	              .returnCode,
	          PLDT::EReturnCode::SUCCESS);
	{
		QpiContextSystemProcedureCall qpi(PLDT_CONTRACT_INDEX, BEGIN_TICK);
		ASSERT_GE(qpi.transferShareOwnershipAndPossession(currency.assetName, currency.issuer, CREATOR, CREATOR, 50, OUTSIDER), 0);
	}
	setCalendar(2025, 1, 4);
	processFirstGameAt(100);
	ASSERT_EQ(game(created.gameId).game.status, PLDT::EGameStatus::FINALIZING);
	EXPECT_TRUE(game(created.gameId).game.pendingEconomics.isSet);
	EXPECT_EQ(wallet(CREATOR).assets.get(0).balance, 150ULL);
	EXPECT_EQ(wallet(CREATOR).serviceCredit, 988000ULL);
	EXPECT_EQ(platformAccounting().developer1Accrued, 2500ULL);
	EXPECT_EQ(platformAccounting().resultCounter, 0ULL);
	processFirstGameAt(200);
	EXPECT_EQ(wallet(CREATOR).assets.get(0).balance, 150ULL);
	EXPECT_EQ(wallet(CREATOR).serviceCredit, 988000ULL);
	{
		QpiContextSystemProcedureCall qpi(PLDT_CONTRACT_INDEX, BEGIN_TICK);
		ASSERT_GE(qpi.transferShareOwnershipAndPossession(currency.assetName, currency.issuer, OUTSIDER, OUTSIDER, 50, CREATOR), 0);
	}
	processFirstGameAt(300);
	ASSERT_EQ(game(created.gameId).game.roundNumber, 2ULL);
	EXPECT_EQ(game(created.gameId).game.prizePool, 150ULL);
	EXPECT_FALSE(game(created.gameId).game.pendingEconomics.isSet);
	EXPECT_EQ(wallet(CREATOR).assets.get(0).balance, 0ULL);
	EXPECT_EQ(wallet(CREATOR).serviceCredit, 978000ULL);
	EXPECT_EQ(platformAccounting().resultCounter, 1ULL);
	EXPECT_EQ(result(created.gameId).gameResult.prizePool, 100ULL);
	processFirstGameAt(400);
	EXPECT_EQ(wallet(CREATOR).serviceCredit, 978000ULL);
	EXPECT_EQ(platformAccounting().resultCounter, 1ULL);
}

TEST_F(ContractTestingPulseEditorV3, SharedWalletZeroAssetSeedDoesNotTransferShares)
{
	const Asset currency{CREATOR, assetNameFromString("PEDZERO")};
	ASSERT_EQ(issueAsset(currency, 1), 1);
	ASSERT_EQ(transferAssetManagement(currency, CREATOR, 1, PLDT_CONTRACT_INDEX), 1);
	auto input = makeGame(2, 0);
	input.mode = PLDT::EGameMode::PERMANENT;
	input.currencyMode = PLDT::ECurrencyMode::ASSET;
	input.currencyAsset = currency;
	input.ownershipManagingContractIndex = PLDT_CONTRACT_INDEX;
	input.possessionManagingContractIndex = PLDT_CONTRACT_INDEX;
	const auto created = createGame(CREATOR, input);
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
	setCalendar(2025, 1, 4);
	processFirstGameAt(100);
	EXPECT_EQ(game(created.gameId).game.roundNumber, 2ULL);
	EXPECT_EQ(wallet(CREATOR).assets.get(0).balance, 1ULL);
	EXPECT_EQ(wallet(CREATOR).serviceCredit, 979000ULL);
	EXPECT_EQ(numberOfPossessedShares(currency.assetName, currency.issuer, contractId(), contractId(), PLDT_CONTRACT_INDEX, PLDT_CONTRACT_INDEX), 0);
}

TEST_F(ContractTestingPulseEditorV3, PermanentCreationLeavesFutureFundsInWallet)
{
	ASSERT_EQ(createWallet(CREATOR, PLDT_DEFAULT_WALLET_CREATION_FEE + 500).returnCode, PLDT::EReturnCode::SUCCESS);
	auto input = makeGame(2, 100);
	input.mode = PLDT::EGameMode::PERMANENT;
	const auto created = createGame(CREATOR, input);
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
	const auto stored = game(created.gameId).game;
	EXPECT_EQ(stored.mode, PLDT::EGameMode::PERMANENT);
	EXPECT_EQ(stored.roundNumber, 1ULL);
	EXPECT_EQ(stored.prizePool, 100ULL);
	EXPECT_EQ(stored.roundFeeSnapshot, 10000ULL);
	EXPECT_EQ(wallet(CREATOR).refundableQubic, 400ULL);
	EXPECT_EQ(wallet(CREATOR).serviceCredit, 989000ULL);
}

TEST_F(ContractTestingPulseEditorV3, WalletWithdrawalDoesNotWithdrawCurrentPrizePool)
{
	ASSERT_EQ(createWallet(CREATOR, PLDT_DEFAULT_WALLET_CREATION_FEE + 300).returnCode, PLDT::EReturnCode::SUCCESS);
	auto input = makeGame(2, 100);
	input.mode = PLDT::EGameMode::PERMANENT;
	const auto created = createGame(CREATOR, input);
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
	depositWalletQubic(CREATOR, 50);
	PLDT::WithdrawWalletQubic_input withdrawal{};
	withdrawal.amount = 251;
	EXPECT_EQ(
	    (procedure<PLDT::WithdrawWalletQubic_input, PLDT::WithdrawWalletQubic_output>(PLDT_PROCEDURE_WITHDRAW_WALLET_QUBIC, withdrawal, CREATOR))
	        .returnCode,
	    PLDT::EReturnCode::INSUFFICIENT_FUNDS);
	withdrawal.amount = 250;
	EXPECT_EQ(
	    (procedure<PLDT::WithdrawWalletQubic_input, PLDT::WithdrawWalletQubic_output>(PLDT_PROCEDURE_WITHDRAW_WALLET_QUBIC, withdrawal, CREATOR))
	        .returnCode,
	    PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(wallet(CREATOR).refundableQubic, 0ULL);
	EXPECT_EQ(game(created.gameId).game.prizePool, 100ULL);
}

TEST_F(ContractTestingPulseEditorV3, PermanentNoTicketRoundReturnsSeedAndAppliesPendingEconomicsOnImmediateRollover)
{
	PLDT::CreateGame_input createInput{};
	createInput = makeGame(2, 100);
	createInput.mode = PLDT::EGameMode::PERMANENT;
	const auto created = createGame(CREATOR, createInput);
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);

	depositWalletQubic(CREATOR, 100);
	PLDT::UpdateGameEconomics_input update{};
	update.gameId = created.gameId;
	update.ticketPrice = 250;
	update.creatorPrizeSeed = 150;
	update.ticketLimit = 5;
	update.playerTicketLimit = 3;
	update.creatorFeePercent = 10;
	const auto updated =
	    procedure<PLDT::UpdateGameEconomics_input, PLDT::UpdateGameEconomics_output>(PLDT_PROCEDURE_UPDATE_GAME_ECONOMICS, update, CREATOR);
	ASSERT_EQ(updated.returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(game(created.gameId).game.ticketPrice, 100ULL);

	setCalendar(2025, 1, 4);
	processFirstGameAt(100);
	const auto next = game(created.gameId).game;
	EXPECT_EQ(next.roundNumber, 2ULL);
	EXPECT_EQ(next.ticketPrice, 250ULL);
	EXPECT_EQ(next.creatorPrizeSeed, 150ULL);
	EXPECT_EQ(next.prizePool, 150ULL);
	EXPECT_EQ(wallet(CREATOR).refundableQubic, 50ULL);
	EXPECT_EQ(next.startAt, DateAndTime(2025, 1, 4, 0, 0, 0));
	EXPECT_EQ(next.drawAt, DateAndTime(2025, 1, 5, 0, 0, 0));
	EXPECT_EQ(next.status, PLDT::EGameStatus::SELLING);
}

TEST_F(ContractTestingPulseEditorV3, PermanentStopsOnceWhenNextExactDrawExceedsDateRange)
{
	PLDT::CreateGame_input input{};
	input = makeGame(2, 100);
	input.mode = PLDT::EGameMode::PERMANENT;
	const auto created = createGame(CREATOR, input);
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);

	auto* contractState = reinterpret_cast<PLDT::StateData*>(contractStates[PLDT_CONTRACT_INDEX]);
	auto storedGame = contractState->games.get(created.slot);
	storedGame.roundDurationMicroseconds = 9223372036854775807ULL;
	contractState->games.set(created.slot, storedGame);
	setCalendar(2025, 1, 4);
	processFirstGameAt(100);
	EXPECT_EQ(game(created.gameId).returnCode, PLDT::EReturnCode::INVALID_GAME);
	EXPECT_EQ(wallet(CREATOR).serviceCredit, PLDT_DEFAULT_WALLET_CREATION_FEE - PLDT_DEFAULT_ROUND_FEE - PLDT_DEFAULT_OPERATION_FEE);
	const auto completed = result(created.gameId);
	ASSERT_EQ(completed.returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(completed.gameResult.gameStopReason, PLDT::EGameStopReason::SCHEDULE_EXHAUSTED);
	const auto counter = platformAccounting().resultCounter;
	processFirstGameAt(200);
	EXPECT_EQ(platformAccounting().resultCounter, counter);
}

TEST_F(ContractTestingPulseEditorV3, PermanentPreStartStopReturnsSeedButKeepsChargedFee)
{
	PLDT::CreateGame_input createInput{};
	createInput = makeGame(2, 100);
	createInput.mode = PLDT::EGameMode::PERMANENT;
	const auto created = createGame(CREATOR, createInput);
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
	const auto balanceBefore = wallet(CREATOR);

	PLDT::StopGame_input stopInput{};
	stopInput.gameId = created.gameId;
	const auto stopped = procedure<PLDT::StopGame_input, PLDT::StopGame_output>(PLDT_PROCEDURE_STOP_GAME, stopInput, CREATOR);

	EXPECT_EQ(stopped.returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(wallet(CREATOR).serviceCredit + wallet(CREATOR).refundableQubic,
	          balanceBefore.serviceCredit + balanceBefore.refundableQubic + 100 - PLDT_DEFAULT_OPERATION_FEE);
	EXPECT_EQ(game(created.gameId).returnCode, PLDT::EReturnCode::INVALID_GAME);
	const auto completed = result(created.gameId);
	ASSERT_EQ(completed.returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(completed.gameResult.terminalReason, PLDT::EGameTerminalReason::OWNER_CANCELLED);
	EXPECT_EQ(completed.gameResult.gameStopReason, PLDT::EGameStopReason::OWNER_REQUESTED);
}

TEST_F(ContractTestingPulseEditorV3, PermanentStopsOutOfFundsAfterCompletingCurrentNoTicketRound)
{
	reinterpret_cast<PLDT::StateData*>(contractStates[PLDT_CONTRACT_INDEX])->walletCreationFee = 10100;
	ASSERT_EQ(createWallet(CREATOR, 10200).returnCode, PLDT::EReturnCode::SUCCESS);
	PLDT::CreateGame_input createInput{};
	createInput = makeGame(2, 100);
	createInput.mode = PLDT::EGameMode::PERMANENT;
	const auto created = createGame(CREATOR, createInput);
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);

	setCalendar(2025, 1, 4);
	processFirstGameAt(100);
	EXPECT_EQ(game(created.gameId).returnCode, PLDT::EReturnCode::INVALID_GAME);
	EXPECT_EQ(wallet(CREATOR).serviceCredit, 0ULL);
	const auto completed = result(created.gameId);
	ASSERT_EQ(completed.returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(completed.gameResult.gameStopReason, PLDT::EGameStopReason::OUT_OF_FUNDS);
}

TEST_F(ContractTestingPulseEditorV3, PermanentNoWinnerReturnedPoolFundsNextRound)
{
	PLDT::CreateGame_input input{};
	input = makeGame(1, 100);
	input.mode = PLDT::EGameMode::PERMANENT;
	input.tierWeightsBps = {};
	input.tierWeightsBps.set(PLDT::payoutMatrixIndex(4, 0), 10000);
	input.maxDigit = PLDT_MIN_MAX_DIGIT;
	input.creatorFeePercent = 10;
	const auto created = createGame(CREATOR, input);
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
	const m256i digest(9, 10, 11, 12);
	etalonTick.prevSpectrumDigest = digest;
	const auto winning = expectedWinningDigits(digest, created.gameId, 1, input.codeLength, input.maxDigit, input.allowRepeatedDigits);
	const auto losing = nonMatchingDigits(winning, input.codeLength, input.maxDigit);

	setCalendar(2025, 1, 3);
	ASSERT_EQ(buyDigits(PLAYER, created.gameId, losing).returnCode, PLDT::EReturnCode::SUCCESS);
	beginTickAt(100);

	EXPECT_EQ(game(created.gameId).game.roundNumber, 2ULL);
	EXPECT_EQ(wallet(CREATOR).refundableQubic, 97ULL);
	const auto completed = result(created.gameId);
	ASSERT_EQ(completed.returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(completed.gameResult.gameStopReason, PLDT::EGameStopReason::NONE);
	EXPECT_EQ(completed.gameResult.terminalReason, PLDT::EGameTerminalReason::NO_WINNERS);
	EXPECT_EQ(completed.gameResult.prizePool, 188ULL);
}

TEST_F(ContractTestingPulseEditorV3, RoundResultUsesGameAndRoundIdentityWhilePermanentGameContinues)
{
	PLDT::CreateGame_input input{};
	input = makeGame(2, 100);
	input.mode = PLDT::EGameMode::PERMANENT;
	const auto created = createGame(CREATOR, input);
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
	setCalendar(2025, 1, 4);
	processFirstGameAt(100);
	ASSERT_EQ(game(created.gameId).game.roundNumber, 2ULL);

	PLDT::GetRoundResult_input resultInput{};
	resultInput.roundKey.gameId = created.gameId;
	resultInput.roundKey.roundNumber = 1;
	const auto first = function<PLDT::GetRoundResult_input, PLDT::GetRoundResult_output>(PLDT_FUNCTION_GET_ROUND_RESULT, resultInput);
	ASSERT_EQ(first.returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(first.roundResult.gameId, created.gameId);
	EXPECT_EQ(first.roundResult.roundNumber, 1ULL);
	EXPECT_TRUE(first.roundResult.detailsAvailable);

	resultInput.roundKey.roundNumber = 2;
	const auto missing = function<PLDT::GetRoundResult_input, PLDT::GetRoundResult_output>(PLDT_FUNCTION_GET_ROUND_RESULT, resultInput);
	EXPECT_EQ(missing.returnCode, PLDT::EReturnCode::INVALID_ROUND);
}

TEST_F(ContractTestingPulseEditorV3, PermanentPreviewDerivesFirstRoundFunding)
{
	PLDT::PreviewGame_input input{};
	input = makeGame(2, 100);
	const auto valid = function<PLDT::PreviewGame_input, PLDT::PreviewGame_output>(PLDT_FUNCTION_PREVIEW_GAME, input);
	ASSERT_EQ(valid.returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(valid.roundFee, 10000ULL);
	EXPECT_EQ(valid.initialQubicRequired, 10100ULL);

	EXPECT_EQ(valid.operationFee, 1000ULL);
	EXPECT_EQ(valid.initialCreatorAssetRequired, 0ULL);
}

TEST_F(ContractTestingPulseEditorV3, ReclamationExpiresRoundDetailsAndTicketGenerationRejectsReusedId)
{
	PLDT::CreateGame_input input{};
	input = makeGame(1, 100);
	input.mode = PLDT::EGameMode::PERMANENT;
	input.tierWeightsBps = {};
	input.tierWeightsBps.set(PLDT::payoutMatrixIndex(0, 0), 10000);
	input.maxDigit = PLDT_MIN_MAX_DIGIT;
	const auto created = createGame(CREATOR, input);
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
	depositWalletQubic(CREATOR, 200);
	setCalendar(2025, 1, 3);
	const auto first = buy(PLAYER, created.gameId, 0);
	ASSERT_EQ(first.returnCode, PLDT::EReturnCode::SUCCESS);
	ASSERT_NE(first.ticketIds.get(0), 0ULL);
	beginTickAt(100);

	auto* contractState = reinterpret_cast<PLDT::StateData*>(contractStates[PLDT_CONTRACT_INDEX]);
	contractState->nextUnusedTicketSlot = PLDT_MAX_TICKETS;
	beginTickAt(200);

	PLDT::GetRoundResult_input resultInput{};
	resultInput.roundKey.gameId = created.gameId;
	resultInput.roundKey.roundNumber = 1;
	const auto expired = function<PLDT::GetRoundResult_input, PLDT::GetRoundResult_output>(PLDT_FUNCTION_GET_ROUND_RESULT, resultInput);
	ASSERT_EQ(expired.returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_FALSE(expired.roundResult.detailsAvailable);

	const auto second = buy(SECOND_PLAYER, created.gameId, 0);
	ASSERT_EQ(second.returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_NE(second.ticketIds.get(0), first.ticketIds.get(0));
	PLDT::GetTicket_input staleInput{};
	staleInput.ticketId = first.ticketIds.get(0);
	const auto stale = function<PLDT::GetTicket_input, PLDT::GetTicket_output>(PLDT_FUNCTION_GET_TICKET, staleInput);
	EXPECT_EQ(stale.returnCode, PLDT::EReturnCode::INVALID_TICKET);

	PLDT::GetPlayerTickets_input pageInput{};
	pageInput.player = PLAYER;
	pageInput.roundKey.gameId = created.gameId;
	pageInput.roundKey.roundNumber = 1;
	pageInput.limit = 10;
	const auto page = function<PLDT::GetPlayerTickets_input, PLDT::GetPlayerTickets_output>(PLDT_FUNCTION_GET_PLAYER_TICKETS, pageInput);
	EXPECT_EQ(page.returnCode, PLDT::EReturnCode::HISTORY_EXPIRED);
	EXPECT_EQ(players(created.gameId, 1).returnCode, PLDT::EReturnCode::HISTORY_EXPIRED);
}

TEST_F(ContractTestingPulseEditorV3, ResultStorageBackpressureReclaimsTicketsBeforeOverwritingTheirSummary)
{
	auto ticketGameInput = makeGame(1, 100);
	auto laterGameInput = makeGame(1, 0);
	laterGameInput.startAt = DateAndTime(2025, 1, 4, 0, 0, 0);
	laterGameInput.drawAt = DateAndTime(2025, 1, 5, 0, 0, 0);
	const auto ticketGame = createGame(CREATOR, ticketGameInput);
	const auto laterGame = createGame(CREATOR, laterGameInput);
	ASSERT_EQ(ticketGame.returnCode, PLDT::EReturnCode::SUCCESS);
	ASSERT_EQ(laterGame.returnCode, PLDT::EReturnCode::SUCCESS);

	setCalendar(2025, 1, 3);
	ASSERT_EQ(buy(PLAYER, ticketGame.gameId, 0).returnCode, PLDT::EReturnCode::SUCCESS);
	setCalendar(2025, 1, 4);
	processFirstGameAt(100);
	auto* contractState = reinterpret_cast<PLDT::StateData*>(contractStates[PLDT_CONTRACT_INDEX]);
	ASSERT_EQ(contractState->resultCounter, 1ULL);
	ASSERT_EQ(contractState->freeTicketCount, 0U);

	// Simulate a long summary-only backlog while the oldest ticket details still await reclamation.
	contractState->resultCounter = PLDT_RESULT_STORAGE_SIZE;
	contractState->reclaimResultCounter = 0;
	setCalendar(2025, 1, 5);
	processFirstGameAt(200);

	EXPECT_EQ(contractState->freeTicketCount, 1U);
	processFirstGameAt(300);
	EXPECT_EQ(game(laterGame.gameId).returnCode, PLDT::EReturnCode::INVALID_GAME);
}

TEST_F(ContractTestingPulseEditorV3, PermanentFinalizationReturnsQubicToWalletWithoutExternalTransfer)
{
	PLDT::CreateGame_input input{};
	input = makeGame(2, 100);
	input.mode = PLDT::EGameMode::PERMANENT;
	const auto created = createGame(CREATOR, input);
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
	PLDT::StopGame_input stopInput{};
	stopInput.gameId = created.gameId;
	const auto stopped = procedure<PLDT::StopGame_input, PLDT::StopGame_output>(PLDT_PROCEDURE_STOP_GAME, stopInput, CREATOR);
	ASSERT_EQ(stopped.returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(wallet(CREATOR).serviceCredit, PLDT_DEFAULT_WALLET_CREATION_FEE - PLDT_DEFAULT_ROUND_FEE - (2 * PLDT_DEFAULT_OPERATION_FEE));
	EXPECT_EQ(game(created.gameId).returnCode, PLDT::EReturnCode::INVALID_GAME);
}

TEST_F(ContractTestingPulseEditorV3, PermanentEconomicsAreFrozenDuringRetryableFinalization)
{
	PLDT::CreateGame_input input{};
	input = makeGame(2, 100);
	input.mode = PLDT::EGameMode::PERMANENT;
	const auto created = createGame(CREATOR, input);
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
	auto* contractState = reinterpret_cast<PLDT::StateData*>(contractStates[PLDT_CONTRACT_INDEX]);
	auto finalizingGame = contractState->games.get(created.slot);
	finalizingGame.status = PLDT::EGameStatus::FINALIZING;
	contractState->games.set(created.slot, finalizingGame);
	ASSERT_EQ(game(created.gameId).game.status, PLDT::EGameStatus::FINALIZING);

	PLDT::UpdateGameEconomics_input updateInput{};
	updateInput.gameId = created.gameId;
	updateInput.ticketPrice = 100;
	updateInput.creatorPrizeSeed = 100;
	updateInput.ticketLimit = 2;
	updateInput.playerTicketLimit = 2;
	const auto updated =
	    procedure<PLDT::UpdateGameEconomics_input, PLDT::UpdateGameEconomics_output>(PLDT_PROCEDURE_UPDATE_GAME_ECONOMICS, updateInput, CREATOR);
	EXPECT_EQ(updated.returnCode, PLDT::EReturnCode::INVALID_STATE);
}

TEST_F(ContractTestingPulseEditorV3, AssetPermanentCreationOnlyReservesFirstSeed)
{
	const Asset currency{CREATOR, assetNameFromString("PEDPERM")};
	ASSERT_EQ(issueAsset(currency, 500), 500);
	ASSERT_EQ(transferAssetManagement(currency, CREATOR, 500, PLDT_CONTRACT_INDEX), 500);
	PLDT::CreateGame_input input{};
	input = makeGame(2, 100);
	input.mode = PLDT::EGameMode::PERMANENT;
	input.currencyMode = PLDT::ECurrencyMode::ASSET;
	input.currencyAsset = currency;
	input.ownershipManagingContractIndex = PLDT_CONTRACT_INDEX;
	input.possessionManagingContractIndex = PLDT_CONTRACT_INDEX;
	fund(CREATOR, 30000);

	const auto created = createGame(CREATOR, input);
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(wallet(CREATOR).serviceCredit, 989000ULL);
	EXPECT_EQ(wallet(CREATOR).assets.get(0).balance, 400ULL);
	EXPECT_EQ(numberOfPossessedShares(currency.assetName, currency.issuer, contractId(), contractId(), PLDT_CONTRACT_INDEX, PLDT_CONTRACT_INDEX),
	          100);
}

TEST_F(ContractTestingPulseEditorV3, AssetPreviewSeparatesQubicFeeFromAssetSeed)
{
	const Asset currency{CREATOR, assetNameFromString("PEDCAP")};
	ASSERT_EQ(issueAsset(currency, 1), 1);
	ASSERT_EQ(transferAssetManagement(currency, CREATOR, 1, PLDT_CONTRACT_INDEX), 1);
	PLDT::CreateGame_input input{};
	input = makeGame(1, 1);
	input.mode = PLDT::EGameMode::PERMANENT;
	input.currencyMode = PLDT::ECurrencyMode::ASSET;
	input.currencyAsset = currency;
	input.ownershipManagingContractIndex = PLDT_CONTRACT_INDEX;
	input.possessionManagingContractIndex = PLDT_CONTRACT_INDEX;
	const auto previewed = function<PLDT::PreviewGame_input, PLDT::PreviewGame_output>(PLDT_FUNCTION_PREVIEW_GAME, input);
	ASSERT_EQ(previewed.returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(previewed.initialQubicRequired, 10000ULL);
	EXPECT_EQ(previewed.initialCreatorAssetRequired, 1ULL);
	const auto created = createGame(CREATOR, input);
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(wallet(CREATOR).serviceCredit, 989000ULL);
	EXPECT_EQ(wallet(CREATOR).assets.get(0).balance, 0ULL);
}

TEST_F(ContractTestingPulseEditorV3, PermanentEconomicsRejectsContractSharePriceThatWouldRequireBurn)
{
	issuePulseEditorSharesTo(PLAYER);
	const Asset contractShares{NULL_ID, assetNameFromString("PLDT")};
	ASSERT_EQ(transferAssetManagement(contractShares, PLAYER, NUMBER_OF_COMPUTORS, PLDT_CONTRACT_INDEX), NUMBER_OF_COMPUTORS);
	PLDT::CreateGame_input input{};
	input = makeGame(1, 1);
	input.mode = PLDT::EGameMode::PERMANENT;
	input.ticketPrice = 1;
	input.currencyMode = PLDT::ECurrencyMode::ASSET;
	input.currencyAsset = contractShares;
	input.ownershipManagingContractIndex = PLDT_CONTRACT_INDEX;
	input.possessionManagingContractIndex = PLDT_CONTRACT_INDEX;
	fund(PLAYER, PLDT_DEFAULT_ROUND_FEE);
	const auto created = createGame(PLAYER, input);
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);

	PLDT::UpdateGameEconomics_input update{};
	update.gameId = created.gameId;
	// At 103 shares, the net is 100 and the 1% burn is no longer rounded to zero.
	update.ticketPrice = 103;
	update.creatorPrizeSeed = 1;
	update.ticketLimit = 1;
	update.playerTicketLimit = 1;
	const auto updated =
	    procedure<PLDT::UpdateGameEconomics_input, PLDT::UpdateGameEconomics_output>(PLDT_PROCEDURE_UPDATE_GAME_ECONOMICS, update, PLAYER);
	EXPECT_EQ(updated.returnCode, PLDT::EReturnCode::INVALID_VALUE);
}

TEST_F(ContractTestingPulseEditorV3, AssetPermanentNoTicketOutOfFundsReturnsSeedAndReleasesGame)
{
	reinterpret_cast<PLDT::StateData*>(contractStates[PLDT_CONTRACT_INDEX])->walletCreationFee = 10100;
	ASSERT_EQ(createWallet(CREATOR, 10200).returnCode, PLDT::EReturnCode::SUCCESS);
	const Asset currency{CREATOR, assetNameFromString("PEDSTOP")};
	ASSERT_EQ(issueAsset(currency, 100), 100);
	ASSERT_EQ(transferAssetManagement(currency, CREATOR, 100, PLDT_CONTRACT_INDEX), 100);
	PLDT::CreateGame_input input{};
	input = makeGame(1, 100);
	input.mode = PLDT::EGameMode::PERMANENT;
	input.currencyMode = PLDT::ECurrencyMode::ASSET;
	input.currencyAsset = currency;
	input.ownershipManagingContractIndex = PLDT_CONTRACT_INDEX;
	input.possessionManagingContractIndex = PLDT_CONTRACT_INDEX;
	fund(CREATOR, PLDT_DEFAULT_ROUND_FEE);
	const auto created = createGame(CREATOR, input);
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(numberOfPossessedShares(currency.assetName, currency.issuer, contractId(), contractId(), PLDT_CONTRACT_INDEX, PLDT_CONTRACT_INDEX),
	          100);

	setCalendar(2025, 1, 4);
	processFirstGameAt(100);
	EXPECT_EQ(game(created.gameId).returnCode, PLDT::EReturnCode::INVALID_GAME);
	EXPECT_EQ(result(created.gameId).roundResult.gameStopReason, PLDT::EGameStopReason::OUT_OF_FUNDS);
	EXPECT_EQ(numberOfPossessedShares(currency.assetName, currency.issuer, CREATOR, CREATOR, PLDT_CONTRACT_INDEX, PLDT_CONTRACT_INDEX), 100);
	EXPECT_EQ(numberOfPossessedShares(currency.assetName, currency.issuer, contractId(), contractId(), PLDT_CONTRACT_INDEX, PLDT_CONTRACT_INDEX), 0);
}

TEST_F(ContractTestingPulseEditorV3, ActivePermanentStopWaitsForConfiguredRoundCompletion)
{
	PLDT::CreateGame_input input{};
	input = makeGame(2, 100);
	input.mode = PLDT::EGameMode::PERMANENT;
	const auto created = createGame(CREATOR, input);
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
	setCalendar(2025, 1, 3);
	PLDT::StopGame_input stopInput{};
	stopInput.gameId = created.gameId;
	const auto stopped = procedure<PLDT::StopGame_input, PLDT::StopGame_output>(PLDT_PROCEDURE_STOP_GAME, stopInput, CREATOR);
	ASSERT_EQ(stopped.returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_TRUE(game(created.gameId).game.stopRequested);

	setCalendar(2025, 1, 4);
	processFirstGameAt(100);
	EXPECT_EQ(game(created.gameId).returnCode, PLDT::EReturnCode::INVALID_GAME);
	EXPECT_EQ(wallet(CREATOR).serviceCredit, PLDT_DEFAULT_WALLET_CREATION_FEE - PLDT_DEFAULT_ROUND_FEE - (2 * PLDT_DEFAULT_OPERATION_FEE));
}

TEST_F(ContractTestingPulseEditorV3, PlatformRoundFeeChangeLeavesCurrentSnapshotsAndAffectsNewGames)
{
	PLDT::CreateGame_input permanentInput{};
	permanentInput = makeGame(2, 100);
	permanentInput.mode = PLDT::EGameMode::PERMANENT;
	const auto existing = createGame(CREATOR, permanentInput);
	ASSERT_EQ(existing.returnCode, PLDT::EReturnCode::SUCCESS);

	issuePulseEditorSharesTo(PLAYER);
	const auto proposal = proposeEconomics(PLAYER, 3, 20000);
	ASSERT_NE(static_cast<uint32>(proposal), static_cast<uint32>(INVALID_PROPOSAL_INDEX));
	ASSERT_TRUE(voteEconomics(PLAYER, proposal));
	callSystemProcedure(PLDT_CONTRACT_INDEX, END_EPOCH);
	EXPECT_EQ(game(existing.gameId).game.roundFeeSnapshot, 10000ULL);

	auto newerInput = makeGame(1, 100);
	const auto newer = createGame(SECOND_PLAYER, newerInput);
	ASSERT_EQ(newer.returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(game(newer.gameId).game.roundFeeSnapshot, 20000ULL);
}

TEST_F(ContractTestingPulseEditorV3, StopAfterTicketCapStillPreventsNextRound)
{
	PLDT::CreateGame_input input{};
	input = makeGame(1, 100);
	input.mode = PLDT::EGameMode::PERMANENT;
	input.tierWeightsBps = {};
	input.tierWeightsBps.set(PLDT::payoutMatrixIndex(0, 0), 10000);
	input.maxDigit = PLDT_MIN_MAX_DIGIT;
	input.creatorFeePercent = 10;
	const auto created = createGame(CREATOR, input);
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
	setCalendar(2025, 1, 3);
	ASSERT_EQ(buy(PLAYER, created.gameId, 0).returnCode, PLDT::EReturnCode::SUCCESS);
	PLDT::StopGame_input stopInput{};
	stopInput.gameId = created.gameId;
	const auto stopped = procedure<PLDT::StopGame_input, PLDT::StopGame_output>(PLDT_PROCEDURE_STOP_GAME, stopInput, CREATOR);
	ASSERT_EQ(stopped.returnCode, PLDT::EReturnCode::SUCCESS);
	setCalendar(2025, 1, 4);
	processFirstGameAt(100);
	EXPECT_EQ(game(created.gameId).returnCode, PLDT::EReturnCode::INVALID_GAME);
	EXPECT_EQ(result(created.gameId).gameResult.gameStopReason, PLDT::EGameStopReason::OWNER_REQUESTED);
}

TEST_F(ContractTestingPulseEditorV3, ActivePermanentStopQueuesClosureAfterCurrentRound)
{
	PLDT::CreateGame_input input{};
	input = makeGame(2, 100);
	input.mode = PLDT::EGameMode::PERMANENT;
	const auto created = createGame(CREATOR, input);
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
	setCalendar(2025, 1, 3);
	EXPECT_EQ(stop(CREATOR, created.gameId).returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_TRUE(game(created.gameId).game.stopRequested);
}

TEST_F(ContractTestingPulseEditorV3, OneShotReturnsSeedAndNeverChargesAnotherRound)
{
	auto input = makeGame(1, 100);
	const auto created = createGame(CREATOR, input);
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(game(created.gameId).game.prizePool, 100ULL);
	EXPECT_EQ(wallet(CREATOR).refundableQubic, 0ULL);

	setCalendar(2025, 1, 4);
	processFirstGameAt(100);
	EXPECT_EQ(game(created.gameId).returnCode, PLDT::EReturnCode::INVALID_GAME);
	EXPECT_EQ(wallet(CREATOR).serviceCredit, PLDT_DEFAULT_WALLET_CREATION_FEE - PLDT_DEFAULT_ROUND_FEE - PLDT_DEFAULT_OPERATION_FEE);
	EXPECT_EQ(wallet(CREATOR).refundableQubic, 100ULL);
	EXPECT_EQ(result(created.gameId).gameResult.gameStopReason, PLDT::EGameStopReason::ONE_SHOT_COMPLETE);
}

TEST_F(ContractTestingPulseEditorV3, OneShotRejectsEconomicsUpdate)
{
	const auto created = createGame(CREATOR, makeGame(1, 100));
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
	PLDT::UpdateGameEconomics_input update{};
	update.gameId = created.gameId;
	const auto updated =
	    procedure<PLDT::UpdateGameEconomics_input, PLDT::UpdateGameEconomics_output>(PLDT_PROCEDURE_UPDATE_GAME_ECONOMICS, update, CREATOR);
	EXPECT_EQ(updated.returnCode, PLDT::EReturnCode::INVALID_STATE);
}

TEST_F(ContractTestingPulseEditorV3, OneCreatorCannotOccupyMoreThanTheActiveGameQuota)
{
	for (uint16 i = 0; i < PLDT_MAX_ACTIVE_GAMES_PER_CREATOR; ++i)
	{
		ASSERT_EQ(createGame(CREATOR, makeGame(1, 0)).returnCode, PLDT::EReturnCode::SUCCESS);
	}

	EXPECT_EQ(createGame(CREATOR, makeGame(1, 0)).returnCode, PLDT::EReturnCode::STORAGE_FULL);
	EXPECT_EQ(static_cast<uint32>(games().totalActive), static_cast<uint32>(PLDT_MAX_ACTIVE_GAMES_PER_CREATOR));
}

TEST_F(ContractTestingPulseEditorV3, CreateRefundsSeedWhenAllGlobalGameSlotsAreOccupied)
{
	auto* contractState = reinterpret_cast<PLDT::StateData*>(contractStates[PLDT_CONTRACT_INDEX]);
	PLDT::Game occupied{};
	occupied.owner = OUTSIDER;
	occupied.status = PLDT::EGameStatus::SCHEDULED;
	for (uint16 i = 0; i < PLDT_MAX_GAMES; ++i)
	{
		contractState->games.set(i, occupied);
	}
	contractState->activeGameCount = PLDT_MAX_GAMES;
	fund(CREATOR, 10100);
	const sint64 balanceBefore = getBalance(CREATOR);

	const auto created = createGame(CREATOR, makeGame(1, 100));

	EXPECT_EQ(created.returnCode, PLDT::EReturnCode::STORAGE_FULL);
	EXPECT_EQ(getBalance(CREATOR), balanceBefore);
}

TEST_F(ContractTestingPulseEditorV3, BuyIsRejectedBeforeStartAndAcceptedAtStart)
{
	fund(CREATOR, 100);
	const auto created = createGame(CREATOR, makeGame());
	fund(PLAYER, 200);
	const sint64 before = getBalance(PLAYER);
	EXPECT_EQ(buy(PLAYER, created.gameId, 0).returnCode, PLDT::EReturnCode::GAME_NOT_STARTED);
	EXPECT_EQ(getBalance(PLAYER), before);

	setCalendar(2025, 1, 3);
	const auto accepted = buy(PLAYER, created.gameId, 0);
	EXPECT_EQ(accepted.returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(game(created.gameId).game.status, PLDT::EGameStatus::SELLING);
	EXPECT_EQ(game(created.gameId).game.prizePool, 197);
}

TEST_F(ContractTestingPulseEditorV3, BuyAtDrawTimeIsRejectedAndRefunded)
{
	fund(CREATOR, 100);
	const auto created = createGame(CREATOR, makeGame(2, 100));
	setCalendar(2025, 1, 4);
	fund(PLAYER, 100);
	const sint64 balanceBefore = getBalance(PLAYER);

	const auto purchase = buy(PLAYER, created.gameId, 0);

	EXPECT_EQ(purchase.returnCode, PLDT::EReturnCode::GAME_CLOSED);
	EXPECT_EQ(getBalance(PLAYER), balanceBefore);
	EXPECT_EQ(static_cast<uint32>(game(created.gameId).game.ticketCount), 0U);
}

TEST_F(ContractTestingPulseEditorV3, OwnerCanStopBeforeStartAndActiveOneShotStopIsANoop)
{
	fund(CREATOR, 100);
	const sint64 before = getBalance(CREATOR);
	const auto created = createGame(CREATOR, makeGame());
	EXPECT_EQ(stop(OUTSIDER, created.gameId).returnCode, PLDT::EReturnCode::ACCESS_DENIED);
	EXPECT_EQ(stop(CREATOR, created.gameId).returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(getBalance(CREATOR), before);
	EXPECT_EQ(game(created.gameId).returnCode, PLDT::EReturnCode::INVALID_GAME);
	EXPECT_EQ(result(created.gameId).gameResult.terminalReason, PLDT::EGameTerminalReason::OWNER_CANCELLED);

	fund(CREATOR, 100);
	const auto second = createGame(CREATOR, makeGame());
	setCalendar(2025, 1, 3);
	EXPECT_EQ(stop(CREATOR, second.gameId).returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(game(second.gameId).returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_FALSE(game(second.gameId).game.stopRequested);
}

TEST_F(ContractTestingPulseEditorV3, TicketCapDrawRedistributesPoolAcrossWinningTiers)
{
	fund(CREATOR, 100);
	const auto configuration = makeGame(2, 100);
	const auto created = createGame(CREATOR, configuration);
	const m256i digest(1, 2, 3, 4);
	etalonTick.prevSpectrumDigest = digest;
	const auto winning =
	    expectedWinningDigits(digest, created.gameId, 2, configuration.codeLength, configuration.maxDigit, configuration.allowRepeatedDigits);
	const auto losing = nonMatchingDigits(winning, configuration.codeLength, configuration.maxDigit);
	setCalendar(2025, 1, 3);
	fund(PLAYER, 100);
	fund(SECOND_PLAYER, 100);
	const auto first = buyDigits(PLAYER, created.gameId, winning);
	const auto second = buyDigits(SECOND_PLAYER, created.gameId, losing);
	ASSERT_EQ(first.returnCode, PLDT::EReturnCode::SUCCESS);
	ASSERT_EQ(second.returnCode, PLDT::EReturnCode::SUCCESS);

	beginTickAt(100);
	const auto settled = result(created.gameId);
	ASSERT_EQ(settled.returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(settled.gameResult.terminalReason, PLDT::EGameTerminalReason::SETTLED);
	EXPECT_EQ(settled.gameResult.prizePool, 294);
	EXPECT_EQ(settled.gameResult.totalPaid, 294);
	const auto firstPayout = ticket(first.ticketIndexes.get(0)).ticket.payout;
	const auto secondPayout = ticket(second.ticketIndexes.get(0)).ticket.payout;
	EXPECT_EQ(firstPayout + secondPayout, 294);
	EXPECT_TRUE((firstPayout == 88 && secondPayout == 206) || (firstPayout == 206 && secondPayout == 88));
	EXPECT_EQ(game(created.gameId).returnCode, PLDT::EReturnCode::INVALID_GAME);
}

TEST_F(ContractTestingPulseEditorV3, LargestRemainderTieUsesLowerStableTierIndex)
{
	auto configuration = makeGame(2, 1);
	configuration.tierWeightsBps = {};
	configuration.tierWeightsBps.set(PLDT::payoutMatrixIndex(0, 0), 5000);
	configuration.tierWeightsBps.set(PLDT::payoutMatrixIndex(4, 0), 5000);
	fund(CREATOR, 1);
	const auto created = createGame(CREATOR, configuration);
	const m256i digest(5, 6, 7, 8);
	etalonTick.prevSpectrumDigest = digest;
	const auto winning =
	    expectedWinningDigits(digest, created.gameId, 2, configuration.codeLength, configuration.maxDigit, configuration.allowRepeatedDigits);
	const auto losing = nonMatchingDigits(winning, configuration.codeLength, configuration.maxDigit);
	setCalendar(2025, 1, 3);
	fund(PLAYER, 100);
	fund(SECOND_PLAYER, 100);
	const auto first = buyDigits(PLAYER, created.gameId, winning);
	const auto second = buyDigits(SECOND_PLAYER, created.gameId, losing);
	beginTickAt(100);

	const auto firstTicket = ticket(first.ticketIndexes.get(0)).ticket;
	const auto secondTicket = ticket(second.ticketIndexes.get(0)).ticket;
	ASSERT_NE(static_cast<uint32>(firstTicket.tierIndex), static_cast<uint32>(secondTicket.tierIndex));
	if (firstTicket.tierIndex < secondTicket.tierIndex)
	{
		EXPECT_EQ(firstTicket.payout, 98);
		EXPECT_EQ(secondTicket.payout, 97);
	}
	else
	{
		EXPECT_EQ(firstTicket.payout, 97);
		EXPECT_EQ(secondTicket.payout, 98);
	}
	EXPECT_EQ(result(created.gameId).gameResult.totalPaid, 195);
}

TEST_F(ContractTestingPulseEditorV3, EmptyTiersAreExcludedSoSingleWinnerReceivesWholePool)
{
	fund(CREATOR, 100);
	const auto configuration = makeGame(1, 100);
	const auto created = createGame(CREATOR, configuration);
	const m256i digest(29, 30, 31, 32);
	etalonTick.prevSpectrumDigest = digest;
	const auto winning =
	    expectedWinningDigits(digest, created.gameId, 1, configuration.codeLength, configuration.maxDigit, configuration.allowRepeatedDigits);
	setCalendar(2025, 1, 3);
	fund(PLAYER, 100);
	const auto purchase = buyDigits(PLAYER, created.gameId, winning);
	beginTickAt(100);
	EXPECT_EQ(ticket(purchase.ticketIndexes.get(0)).ticket.payout, 197);
	EXPECT_EQ(result(created.gameId).gameResult.totalPaid, 197);
}

TEST_F(ContractTestingPulseEditorV3, DrawDateWithNoTicketsReturnsSeedWithoutGeneratingResult)
{
	fund(CREATOR, 100);
	const sint64 before = getBalance(CREATOR);
	const auto created = createGame(CREATOR, makeGame(10, 100));
	setCalendar(2025, 1, 4);
	beginTickAt(100);

	const auto finished = result(created.gameId);
	ASSERT_EQ(finished.returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(finished.gameResult.terminalReason, PLDT::EGameTerminalReason::NO_TICKETS);
	EXPECT_EQ(finished.gameResult.totalPaid, 0);
	for (uint16 i = 0; i < PLDT_DIGITS_ALIGNED; ++i)
	{
		EXPECT_EQ(finished.gameResult.winningDigits.get(i), 0);
	}
	EXPECT_EQ(getBalance(CREATOR), before);
}

TEST_F(ContractTestingPulseEditorV3, DrawDateSettlesOneTicketWithoutMinimumPlayerThreshold)
{
	fund(CREATOR, 100);
	const auto configuration = makeGame(10, 100);
	const auto created = createGame(CREATOR, configuration);
	const m256i digest(33, 34, 35, 36);
	etalonTick.prevSpectrumDigest = digest;
	const auto winning =
	    expectedWinningDigits(digest, created.gameId, 1, configuration.codeLength, configuration.maxDigit, configuration.allowRepeatedDigits);
	setCalendar(2025, 1, 3);
	fund(PLAYER, 100);
	const auto purchase = buyDigits(PLAYER, created.gameId, winning);
	setCalendar(2025, 1, 4);
	beginTickAt(100);
	EXPECT_EQ(result(created.gameId).gameResult.terminalReason, PLDT::EGameTerminalReason::SETTLED);
	EXPECT_EQ(ticket(purchase.ticketIndexes.get(0)).ticket.payout, 197);
}

TEST_F(ContractTestingPulseEditorV3, SuccessfulSettlementCreditsCreatorWalletUntilManualWithdrawal)
{
	auto configuration = makeGame(1, 100);
	configuration.creatorFeePercent = 10;
	fund(CREATOR, 100);
	const auto created = createGame(CREATOR, configuration);
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
	const m256i digest(13, 14, 15, 16);
	etalonTick.prevSpectrumDigest = digest;
	const auto winning =
	    expectedWinningDigits(digest, created.gameId, 1, configuration.codeLength, configuration.maxDigit, configuration.allowRepeatedDigits);
	setCalendar(2025, 1, 3);
	fund(PLAYER, 100);
	const auto walletBeforePurchase = wallet(CREATOR);
	const auto purchase = buyDigits(PLAYER, created.gameId, winning);
	ASSERT_EQ(purchase.returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(wallet(CREATOR).refundableQubic, walletBeforePurchase.refundableQubic);
	EXPECT_EQ(game(created.gameId).game.creatorRevenue, 9ULL);
	const auto creatorBalanceBeforeSettlement = getBalance(CREATOR);
	const auto walletBeforeSettlement = wallet(CREATOR);
	beginTickAt(100);

	EXPECT_EQ(result(created.gameId).gameResult.prizePool, 188);
	EXPECT_EQ(ticket(purchase.ticketIndexes.get(0)).ticket.payout, 188);
	EXPECT_EQ(getBalance(CREATOR), creatorBalanceBeforeSettlement);
	EXPECT_EQ(wallet(CREATOR).serviceCredit, walletBeforeSettlement.serviceCredit);
	EXPECT_EQ(wallet(CREATOR).refundableQubic, walletBeforeSettlement.refundableQubic + 9);
	processFirstGameAt(200);
	EXPECT_EQ(wallet(CREATOR).refundableQubic, walletBeforeSettlement.refundableQubic + 9);
	PLDT::WithdrawWalletQubic_input withdraw{};
	withdraw.amount = wallet(CREATOR).serviceCredit + wallet(CREATOR).refundableQubic;
	const auto paid =
	    procedure<PLDT::WithdrawWalletQubic_input, PLDT::WithdrawWalletQubic_output>(PLDT_PROCEDURE_WITHDRAW_WALLET_QUBIC, withdraw, CREATOR);
	ASSERT_EQ(paid.returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(getBalance(CREATOR), creatorBalanceBeforeSettlement + withdraw.amount);
	EXPECT_EQ(wallet(CREATOR).refundableQubic, 0ULL);
}

TEST_F(ContractTestingPulseEditorV3, EarnedCreatorRevenueReturnsToOwnerOnIdleWalletExpiry)
{
	auto configuration = makeGame(1, 0);
	configuration.creatorFeePercent = 10;
	const auto created = createGame(CREATOR, configuration);
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
	const m256i digest(13, 14, 15, 16);
	etalonTick.prevSpectrumDigest = digest;
	const auto winning =
	    expectedWinningDigits(digest, created.gameId, 1, configuration.codeLength, configuration.maxDigit, configuration.allowRepeatedDigits);
	setCalendar(2025, 1, 3);
	ASSERT_EQ(buyDigits(PLAYER, created.gameId, winning).returnCode, PLDT::EReturnCode::SUCCESS);
	const auto externalBefore = getBalance(CREATOR);
	processFirstGameAt(100);
	ASSERT_EQ(game(created.gameId).returnCode, PLDT::EReturnCode::INVALID_GAME);
	ASSERT_EQ(wallet(CREATOR).refundableQubic, 9ULL);
	ASSERT_TRUE(wallet(CREATOR).serviceCreditUnlocked);
	EXPECT_EQ(getBalance(CREATOR), externalBefore);
	const auto unlockedCredit = wallet(CREATOR).serviceCredit;
	const auto accountingBefore = platformAccounting();
	auto* contractState = reinterpret_cast<PLDT::StateData*>(contractStates[PLDT_CONTRACT_INDEX]);
	const auto walletIndex = contractState->wallets.getElementIndex(CREATOR);
	ASSERT_GE(walletIndex, 0);
	contractState->walletAutomationCursor = static_cast<uint16>(walletIndex);
	system.epoch = static_cast<uint16>(system.epoch + 2);
	beginTickAt(200);
	EXPECT_FALSE(wallet(CREATOR).found);
	EXPECT_EQ(getBalance(CREATOR), externalBefore + unlockedCredit + 9);
	EXPECT_EQ(platformAccounting().developer1Accrued, accountingBefore.developer1Accrued);
	EXPECT_EQ(platformAccounting().developer2Accrued, accountingBefore.developer2Accrued);
	EXPECT_EQ(platformAccounting().dividendAccrued, accountingBefore.dividendAccrued);
	contractState->walletAutomationCursor = static_cast<uint16>(walletIndex);
	beginTickAt(300);
	EXPECT_EQ(getBalance(CREATOR), externalBefore + unlockedCredit + 9);
}

TEST_F(ContractTestingPulseEditorV3, FailedManualWithdrawalPreservesEarnedCreatorRevenue)
{
	auto configuration = makeGame(1, 0);
	configuration.mode = PLDT::EGameMode::PERMANENT;
	configuration.creatorFeePercent = 10;
	const auto created = createGame(CREATOR, configuration);
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
	const m256i digest(13, 14, 15, 16);
	etalonTick.prevSpectrumDigest = digest;
	const auto winning =
	    expectedWinningDigits(digest, created.gameId, 1, configuration.codeLength, configuration.maxDigit, configuration.allowRepeatedDigits);
	setCalendar(2025, 1, 3);
	ASSERT_EQ(buyDigits(PLAYER, created.gameId, winning).returnCode, PLDT::EReturnCode::SUCCESS);
	processFirstGameAt(100);
	ASSERT_EQ(game(created.gameId).game.roundNumber, 2ULL);
	ASSERT_EQ(wallet(CREATOR).refundableQubic, 9ULL);
	const auto externalBefore = getBalance(CREATOR);
	const auto serviceBefore = wallet(CREATOR).serviceCredit;
	const auto selfIndex = spectrumIndex(contractId());
	ASSERT_GE(selfIndex, 0);
	const auto custodyBefore = getBalance(contractId());
	ASSERT_TRUE(decreaseEnergy(selfIndex, custodyBefore));
	PLDT::WithdrawWalletQubic_input withdraw{};
	withdraw.amount = 9;
	const auto failed =
	    procedure<PLDT::WithdrawWalletQubic_input, PLDT::WithdrawWalletQubic_output>(PLDT_PROCEDURE_WITHDRAW_WALLET_QUBIC, withdraw, CREATOR);
	EXPECT_EQ(failed.returnCode, PLDT::EReturnCode::TRANSFER_FAILED);
	EXPECT_EQ(failed.amountPaid, 0ULL);
	EXPECT_EQ(wallet(CREATOR).refundableQubic, 9ULL);
	EXPECT_EQ(wallet(CREATOR).serviceCredit, serviceBefore);
	EXPECT_EQ(getBalance(CREATOR), externalBefore);
	increaseEnergy(contractId(), custodyBefore);
	const auto paid =
	    procedure<PLDT::WithdrawWalletQubic_input, PLDT::WithdrawWalletQubic_output>(PLDT_PROCEDURE_WITHDRAW_WALLET_QUBIC, withdraw, CREATOR);
	ASSERT_EQ(paid.returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(paid.amountPaid, 9ULL);
	EXPECT_EQ(wallet(CREATOR).refundableQubic, 0ULL);
	EXPECT_EQ(wallet(CREATOR).serviceCredit, serviceBefore);
	EXPECT_EQ(getBalance(CREATOR), externalBefore + 9);
}

TEST_F(ContractTestingPulseEditorV3, CreatorRevenueWalletCapacityResumesWithoutDoubleCredit)
{
	auto configuration = makeGame(1, 0);
	configuration.creatorFeePercent = 10;
	const auto created = createGame(CREATOR, configuration);
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
	auto* contractState = reinterpret_cast<PLDT::StateData*>(contractStates[PLDT_CONTRACT_INDEX]);
	PLDT::CreatorWallet storedWallet{};
	ASSERT_TRUE(contractState->wallets.get(CREATOR, storedWallet));
	storedWallet.refundableQubic = MAX_AMOUNT - 4;
	contractState->wallets.replace(CREATOR, storedWallet);
	const auto externalBefore = getBalance(CREATOR);
	const m256i digest(13, 14, 15, 16);
	etalonTick.prevSpectrumDigest = digest;
	const auto winning =
	    expectedWinningDigits(digest, created.gameId, 1, configuration.codeLength, configuration.maxDigit, configuration.allowRepeatedDigits);
	setCalendar(2025, 1, 3);
	ASSERT_EQ(buyDigits(PLAYER, created.gameId, winning).returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(wallet(CREATOR).refundableQubic, MAX_AMOUNT - 4);
	processFirstGameAt(100);
	ASSERT_EQ(game(created.gameId).game.status, PLDT::EGameStatus::FINALIZING);
	EXPECT_EQ(game(created.gameId).game.pendingCreatorCurrencyPayout, 5ULL);
	EXPECT_EQ(wallet(CREATOR).refundableQubic, MAX_AMOUNT);
	processFirstGameAt(200);
	EXPECT_EQ(game(created.gameId).game.pendingCreatorCurrencyPayout, 5ULL);
	PLDT::WithdrawWalletQubic_input withdraw{};
	withdraw.amount = 5;
	ASSERT_EQ((procedure<PLDT::WithdrawWalletQubic_input, PLDT::WithdrawWalletQubic_output>(PLDT_PROCEDURE_WITHDRAW_WALLET_QUBIC, withdraw, CREATOR))
	              .returnCode,
	          PLDT::EReturnCode::SUCCESS);
	processFirstGameAt(300);
	EXPECT_EQ(game(created.gameId).returnCode, PLDT::EReturnCode::INVALID_GAME);
	EXPECT_EQ(wallet(CREATOR).refundableQubic, MAX_AMOUNT);
	EXPECT_EQ(getBalance(CREATOR), externalBefore + 5);
	const auto count = platformAccounting().resultCounter;
	processFirstGameAt(400);
	EXPECT_EQ(platformAccounting().resultCounter, count);
	EXPECT_EQ(wallet(CREATOR).refundableQubic, MAX_AMOUNT);
}

TEST_F(ContractTestingPulseEditorV3, AssetPoolReturnWaitsForWalletCapacityAndCustody)
{
	const Asset currency{CREATOR, assetNameFromString("PEDRET")};
	ASSERT_EQ(issueAsset(currency, 10000), 10000);
	ASSERT_EQ(transferAssetManagement(currency, CREATOR, 300, PLDT_CONTRACT_INDEX), 300);
	auto configuration = makeGame(1, 100);
	configuration.currencyMode = PLDT::ECurrencyMode::ASSET;
	configuration.currencyAsset = currency;
	configuration.ownershipManagingContractIndex = PLDT_CONTRACT_INDEX;
	configuration.possessionManagingContractIndex = PLDT_CONTRACT_INDEX;
	const auto created = createGame(CREATOR, configuration);
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
	auto* contractState = reinterpret_cast<PLDT::StateData*>(contractStates[PLDT_CONTRACT_INDEX]);
	PLDT::CreatorWallet storedWallet{};
	ASSERT_TRUE(contractState->wallets.get(CREATOR, storedWallet));
	auto position = storedWallet.assets.get(0);
	position.balance = MAX_AMOUNT - 40;
	storedWallet.assets.set(0, position);
	contractState->wallets.replace(CREATOR, storedWallet);
	setCalendar(2025, 1, 4);
	processFirstGameAt(100);
	ASSERT_EQ(game(created.gameId).game.status, PLDT::EGameStatus::FINALIZING);
	EXPECT_EQ(game(created.gameId).game.pendingPrizePoolPayout, 60ULL);
	EXPECT_EQ(wallet(CREATOR).assets.get(0).balance, MAX_AMOUNT);
	EXPECT_EQ(numberOfPossessedShares(currency.assetName, currency.issuer, CREATOR, CREATOR, PLDT_CONTRACT_INDEX, PLDT_CONTRACT_INDEX), 240);
	processFirstGameAt(200);
	EXPECT_EQ(game(created.gameId).game.pendingPrizePoolPayout, 60ULL);
	// Restore the real ledger after simulating capacity pressure, then make custody temporarily unavailable.
	ASSERT_TRUE(contractState->wallets.get(CREATOR, storedWallet));
	position = storedWallet.assets.get(0);
	position.balance = 240;
	storedWallet.assets.set(0, position);
	contractState->wallets.replace(CREATOR, storedWallet);
	{
		QpiContextSystemProcedureCall qpi(PLDT_CONTRACT_INDEX, BEGIN_TICK);
		ASSERT_GE(qpi.transferShareOwnershipAndPossession(currency.assetName, currency.issuer, contractId(), contractId(), 60, OUTSIDER), 0);
	}
	processFirstGameAt(300);
	EXPECT_EQ(game(created.gameId).game.pendingPrizePoolPayout, 60ULL);
	EXPECT_EQ(wallet(CREATOR).assets.get(0).balance, 240ULL);
	{
		QpiContextSystemProcedureCall qpi(PLDT_CONTRACT_INDEX, BEGIN_TICK);
		ASSERT_GE(qpi.transferShareOwnershipAndPossession(currency.assetName, currency.issuer, OUTSIDER, OUTSIDER, 60, contractId()), 0);
	}
	processFirstGameAt(400);
	EXPECT_EQ(game(created.gameId).returnCode, PLDT::EReturnCode::INVALID_GAME);
	EXPECT_EQ(wallet(CREATOR).assets.get(0).balance, 300ULL);
	EXPECT_EQ(numberOfPossessedShares(currency.assetName, currency.issuer, CREATOR, CREATOR, PLDT_CONTRACT_INDEX, PLDT_CONTRACT_INDEX), 300);
}

TEST_F(ContractTestingPulseEditorV3, AssetCreatorFeeIsNotRepeatedWhenLaterPoolCreditFails)
{
	const Asset currency{CREATOR, assetNameFromString("PEDONCE")};
	ASSERT_EQ(issueAsset(currency, 200), 200);
	ASSERT_EQ(transferAsset(currency, CREATOR, PLAYER, 100), 100);
	ASSERT_EQ(transferAssetManagement(currency, CREATOR, 100, PLDT_CONTRACT_INDEX), 100);
	ASSERT_EQ(transferAssetManagement(currency, PLAYER, 100, PLDT_CONTRACT_INDEX), 100);
	auto configuration = makeGame(1, 100);
	configuration.currencyMode = PLDT::ECurrencyMode::ASSET;
	configuration.currencyAsset = currency;
	configuration.ownershipManagingContractIndex = PLDT_CONTRACT_INDEX;
	configuration.possessionManagingContractIndex = PLDT_CONTRACT_INDEX;
	configuration.creatorFeePercent = 10;
	configuration.tierWeightsBps = {};
	configuration.tierWeightsBps.set(PLDT::payoutMatrixIndex(4, 0), 10000);
	const auto created = createGame(CREATOR, configuration);
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
	const m256i digest(9, 10, 11, 12);
	etalonTick.prevSpectrumDigest = digest;
	const auto winning =
	    expectedWinningDigits(digest, created.gameId, 1, configuration.codeLength, configuration.maxDigit, configuration.allowRepeatedDigits);
	PLDT::BuyTickets_input purchase{};
	purchase.gameId = created.gameId;
	purchase.ticketCount = 1;
	purchase.tickets.set(0, {nonMatchingDigits(winning, configuration.codeLength, configuration.maxDigit)});
	setCalendar(2025, 1, 3);
	ASSERT_EQ((procedure<PLDT::BuyTickets_input, PLDT::BuyTickets_output>(PLDT_PROCEDURE_BUY_TICKETS, purchase, PLAYER)).returnCode,
	          PLDT::EReturnCode::SUCCESS);
	// Leave enough custody for the 9-share fee, but not the subsequent 188-share pool return.
	{
		QpiContextSystemProcedureCall qpi(PLDT_CONTRACT_INDEX, BEGIN_TICK);
		ASSERT_GE(qpi.transferShareOwnershipAndPossession(currency.assetName, currency.issuer, contractId(), contractId(), 188, OUTSIDER), 0);
	}
	processFirstGameAt(100);
	ASSERT_EQ(game(created.gameId).game.status, PLDT::EGameStatus::FINALIZING);
	EXPECT_EQ(wallet(CREATOR).assets.get(0).balance, 9ULL);
	EXPECT_EQ(game(created.gameId).game.pendingCreatorCurrencyPayout, 0ULL);
	EXPECT_EQ(game(created.gameId).game.pendingPrizePoolPayout, 188ULL);
	processFirstGameAt(200);
	EXPECT_EQ(wallet(CREATOR).assets.get(0).balance, 9ULL);
	EXPECT_EQ(numberOfPossessedShares(currency.assetName, currency.issuer, CREATOR, CREATOR, PLDT_CONTRACT_INDEX, PLDT_CONTRACT_INDEX), 9);
	{
		QpiContextSystemProcedureCall qpi(PLDT_CONTRACT_INDEX, BEGIN_TICK);
		ASSERT_GE(qpi.transferShareOwnershipAndPossession(currency.assetName, currency.issuer, OUTSIDER, OUTSIDER, 188, contractId()), 0);
	}
	processFirstGameAt(300);
	EXPECT_EQ(game(created.gameId).returnCode, PLDT::EReturnCode::INVALID_GAME);
	EXPECT_EQ(wallet(CREATOR).assets.get(0).balance, 197ULL);
	EXPECT_EQ(numberOfPossessedShares(currency.assetName, currency.issuer, CREATOR, CREATOR, PLDT_CONTRACT_INDEX, PLDT_CONTRACT_INDEX), 197);
	EXPECT_EQ(result(created.gameId).gameResult.prizePool, 188ULL);
	EXPECT_EQ(platformAccounting().resultCounter, 1ULL);
	processFirstGameAt(400);
	EXPECT_EQ(wallet(CREATOR).assets.get(0).balance, 197ULL);
	EXPECT_EQ(platformAccounting().resultCounter, 1ULL);
}

TEST_F(ContractTestingPulseEditorV3, AssetPermanentRevenueAndNoWinnerPoolCreditWalletBeforeRollover)
{
	const Asset currency{CREATOR, assetNameFromString("PEDREV")};
	ASSERT_EQ(issueAsset(currency, 400), 400);
	ASSERT_EQ(transferAsset(currency, CREATOR, PLAYER, 100), 100);
	ASSERT_EQ(transferAssetManagement(currency, CREATOR, 300, PLDT_CONTRACT_INDEX), 300);
	ASSERT_EQ(transferAssetManagement(currency, PLAYER, 100, PLDT_CONTRACT_INDEX), 100);
	auto configuration = makeGame(1, 100);
	configuration.mode = PLDT::EGameMode::PERMANENT;
	configuration.currencyMode = PLDT::ECurrencyMode::ASSET;
	configuration.currencyAsset = currency;
	configuration.ownershipManagingContractIndex = PLDT_CONTRACT_INDEX;
	configuration.possessionManagingContractIndex = PLDT_CONTRACT_INDEX;
	configuration.creatorFeePercent = 10;
	configuration.tierWeightsBps = {};
	configuration.tierWeightsBps.set(PLDT::payoutMatrixIndex(4, 0), 10000);
	const auto created = createGame(CREATOR, configuration);
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
	const m256i digest(9, 10, 11, 12);
	etalonTick.prevSpectrumDigest = digest;
	const auto winning =
	    expectedWinningDigits(digest, created.gameId, 1, configuration.codeLength, configuration.maxDigit, configuration.allowRepeatedDigits);
	PLDT::BuyTickets_input purchase{};
	purchase.gameId = created.gameId;
	purchase.ticketCount = 1;
	purchase.tickets.set(0, {nonMatchingDigits(winning, configuration.codeLength, configuration.maxDigit)});
	setCalendar(2025, 1, 3);
	ASSERT_EQ((procedure<PLDT::BuyTickets_input, PLDT::BuyTickets_output>(PLDT_PROCEDURE_BUY_TICKETS, purchase, PLAYER)).returnCode,
	          PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(wallet(CREATOR).assets.get(0).balance, 200ULL);
	processFirstGameAt(100);
	ASSERT_EQ(game(created.gameId).game.roundNumber, 2ULL);
	EXPECT_EQ(game(created.gameId).game.prizePool, 100ULL);
	EXPECT_EQ(wallet(CREATOR).assets.get(0).balance, 297ULL);
	EXPECT_EQ(numberOfPossessedShares(currency.assetName, currency.issuer, CREATOR, CREATOR, PLDT_CONTRACT_INDEX, PLDT_CONTRACT_INDEX), 297);
	processFirstGameAt(200);
	EXPECT_EQ(wallet(CREATOR).assets.get(0).balance, 297ULL);
	fund(CREATOR, 100);
	ASSERT_EQ(releaseAssetManagement(CREATOR, currency, 297).returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(wallet(CREATOR).assets.get(0).balance, 0ULL);
	EXPECT_EQ(numberOfPossessedShares(currency.assetName, currency.issuer, CREATOR, CREATOR, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX), 297);
}

TEST_F(ContractTestingPulseEditorV3, BatchIsTheOnlyRegisteredTicketPurchase)
{
	EXPECT_EQ(contractUserProcedures[PLDT_CONTRACT_INDEX][4], nullptr);
	EXPECT_NE(contractUserProcedures[PLDT_CONTRACT_INDEX][15], nullptr);
	EXPECT_EQ(static_cast<uint32>(contractUserProcedureInputSizes[PLDT_CONTRACT_INDEX][15]), sizeof(PLDT::BuyTickets_input));
	EXPECT_EQ(static_cast<uint32>(contractUserProcedureOutputSizes[PLDT_CONTRACT_INDEX][15]), sizeof(PLDT::BuyTickets_output));
}

TEST_F(ContractTestingPulseEditorV3, BatchUsesPerTicketFeeRoundingAndPersistsEveryTicket)
{
	fund(CREATOR, 100);
	const auto created = createGame(CREATOR, makeGame(2, 100));
	setCalendar(2025, 1, 3);
	fund(PLAYER, 200);
	const auto purchase = buyBatch(PLAYER, created.gameId, {0, 1});
	ASSERT_EQ(purchase.returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(static_cast<uint32>(purchase.acceptedCount), 2U);
	EXPECT_EQ(game(created.gameId).game.prizePool, 294);
	beginTickAt(100);
	EXPECT_EQ(result(created.gameId).gameResult.totalPaid, 294);
}

TEST_F(ContractTestingPulseEditorV3, BatchAcceptsOneAndSixteenTicketsWithOrderedIds)
{
	const auto created = createGame(CREATOR, makeGame(17, 0));
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
	setCalendar(2025, 1, 3);
	fund(PLAYER, 1700);
	const auto balanceBefore = getBalance(PLAYER);
	const auto single = buy(PLAYER, created.gameId, 0);
	ASSERT_EQ(single.returnCode, PLDT::EReturnCode::SUCCESS);
	ASSERT_EQ(static_cast<uint32>(single.acceptedCount), 1U);
	ASSERT_NE(single.ticketIds.get(0), 0ULL);
	EXPECT_EQ(ticket(single.ticketIndexes.get(0)).ticket.ticketId, single.ticketIds.get(0));
	for (uint16 i = 1; i < PLDT_MAX_BATCH_TICKETS; ++i)
	{
		EXPECT_EQ(single.ticketIds.get(i), 0ULL);
		EXPECT_EQ(single.ticketIndexes.get(i), 0ULL);
	}

	PLDT::BuyTickets_input input{};
	input.gameId = created.gameId;
	input.ticketCount = 16;
	for (uint16 i = 0; i < 16; ++i)
	{
		input.tickets.set(i, {digits(static_cast<uint8>(i % 8))});
	}
	const auto batch = procedure<PLDT::BuyTickets_input, PLDT::BuyTickets_output>(PLDT_PROCEDURE_BUY_TICKETS, input, PLAYER, 1600);
	ASSERT_EQ(batch.returnCode, PLDT::EReturnCode::SUCCESS);
	ASSERT_EQ(static_cast<uint32>(batch.acceptedCount), 16U);
	for (uint16 i = 0; i < 16; ++i)
	{
		const auto stored = ticket(batch.ticketIndexes.get(i)).ticket;
		EXPECT_EQ(stored.ticketId, batch.ticketIds.get(i));
		EXPECT_EQ(stored.digits.get(0), static_cast<uint8>(i % 8));
		EXPECT_EQ(stored.player, PLAYER);
		EXPECT_NE(batch.ticketIds.get(i), i == 0 ? single.ticketIds.get(0) : batch.ticketIds.get(i - 1));
	}
	EXPECT_EQ(getBalance(PLAYER), balanceBefore - 1700);
	EXPECT_EQ(static_cast<uint32>(game(created.gameId).game.ticketCount), 17U);
	EXPECT_EQ(game(created.gameId).game.prizePool, 1649ULL);
}

TEST_F(ContractTestingPulseEditorV3, BatchChecksPlayerLimitBeforeDigitsForEveryCount)
{
	auto configuration = makeGame(4, 100);
	configuration.playerTicketLimit = 1;
	const auto created = createGame(CREATOR, configuration);
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
	setCalendar(2025, 1, 3);
	fund(PLAYER, 400);
	ASSERT_EQ(buy(PLAYER, created.gameId, 0).returnCode, PLDT::EReturnCode::SUCCESS);
	const auto balanceBefore = getBalance(PLAYER);
	PLDT::BuyTickets_input input{};
	input.gameId = created.gameId;
	input.tickets.set(0, {digits(9)});
	for (uint16 count = 1; count <= 2; ++count)
	{
		input.ticketCount = count;
		const auto rejected = procedure<PLDT::BuyTickets_input, PLDT::BuyTickets_output>(PLDT_PROCEDURE_BUY_TICKETS, input, PLAYER, 100 * count);
		EXPECT_EQ(rejected.returnCode, PLDT::EReturnCode::PLAYER_TICKET_LIMIT);
		EXPECT_EQ(static_cast<uint32>(rejected.acceptedCount), 0U);
		EXPECT_EQ(getBalance(PLAYER), balanceBefore);
		EXPECT_EQ(static_cast<uint32>(game(created.gameId).game.ticketCount), 1U);
		EXPECT_EQ(game(created.gameId).game.prizePool, 197ULL);
	}
}

TEST_F(ContractTestingPulseEditorV3, BatchRejectsCountsOutsideOneThroughSixteenWithoutChangingState)
{
	fund(CREATOR, 100);
	const auto created = createGame(CREATOR, makeGame(20, 100));
	setCalendar(2025, 1, 3);
	fund(PLAYER, 1700);
	const sint64 balanceBefore = getBalance(PLAYER);
	PLDT::BuyTickets_input input{};
	input.gameId = created.gameId;

	const auto empty = procedure<PLDT::BuyTickets_input, PLDT::BuyTickets_output>(PLDT_PROCEDURE_BUY_TICKETS, input, PLAYER);
	input.ticketCount = PLDT_MAX_BATCH_TICKETS + 1;
	const auto oversized = procedure<PLDT::BuyTickets_input, PLDT::BuyTickets_output>(PLDT_PROCEDURE_BUY_TICKETS, input, PLAYER, 1700);

	EXPECT_EQ(empty.returnCode, PLDT::EReturnCode::INVALID_VALUE);
	EXPECT_EQ(oversized.returnCode, PLDT::EReturnCode::INVALID_VALUE);
	EXPECT_EQ(getBalance(PLAYER), balanceBefore);
	EXPECT_EQ(static_cast<uint32>(game(created.gameId).game.ticketCount), 0U);
}

TEST_F(ContractTestingPulseEditorV3, PerPlayerTicketLimitRejectsAdditionalPurchaseAndRefundsIt)
{
	auto configuration = makeGame(3, 100);
	configuration.playerTicketLimit = 1;
	fund(CREATOR, 100);
	const auto created = createGame(CREATOR, configuration);
	setCalendar(2025, 1, 3);
	fund(PLAYER, 200);
	ASSERT_EQ(buy(PLAYER, created.gameId, 0).returnCode, PLDT::EReturnCode::SUCCESS);
	const sint64 balanceBefore = getBalance(PLAYER);

	const auto rejected = buy(PLAYER, created.gameId, 1);

	EXPECT_EQ(rejected.returnCode, PLDT::EReturnCode::PLAYER_TICKET_LIMIT);
	EXPECT_EQ(getBalance(PLAYER), balanceBefore);
	EXPECT_EQ(static_cast<uint32>(game(created.gameId).game.ticketCount), 1U);
}

TEST_F(ContractTestingPulseEditorV3, GlobalLifetimeTicketCapacityRejectsAndRefundsTheNextPurchase)
{
	fund(CREATOR, 100);
	const auto created = createGame(CREATOR, makeGame(2, 100));
	setCalendar(2025, 1, 3);
	auto* contractState = reinterpret_cast<PLDT::StateData*>(contractStates[PLDT_CONTRACT_INDEX]);
	contractState->ticketCount = contractState->tickets.capacity();
	fund(PLAYER, 100);
	const sint64 balanceBefore = getBalance(PLAYER);

	const auto purchase = buy(PLAYER, created.gameId, 0);

	EXPECT_EQ(purchase.returnCode, PLDT::EReturnCode::TICKET_SOLD_OUT);
	EXPECT_EQ(getBalance(PLAYER), balanceBefore);
	EXPECT_EQ(static_cast<uint32>(game(created.gameId).game.ticketCount), 0U);
}

TEST_F(ContractTestingPulseEditorV3, ReusedSlotGetsNewGenerationAwareIdWhileOldResultRemains)
{
	fund(CREATOR, 200);
	const auto first = createGame(CREATOR, makeGame());
	ASSERT_EQ(stop(CREATOR, first.gameId).returnCode, PLDT::EReturnCode::SUCCESS);
	const auto second = createGame(CREATOR, makeGame());
	EXPECT_NE(second.gameId, first.gameId);
	EXPECT_EQ(game(first.gameId).returnCode, PLDT::EReturnCode::INVALID_GAME);
	EXPECT_EQ(result(first.gameId).returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(game(second.gameId).returnCode, PLDT::EReturnCode::SUCCESS);
}

TEST_F(ContractTestingPulseEditorV3, PlatformRejectsCreatorFeeLimitThatCanUnderflowPrizeContribution)
{
	issuePulseEditorSharesTo(PLAYER);
	EXPECT_EQ(static_cast<uint32>(proposeEconomics(PLAYER, 2, 100)), static_cast<uint32>(INVALID_PROPOSAL_INDEX));
	const auto proposal = proposeEconomics(PLAYER, 2, 99);
	ASSERT_NE(static_cast<uint32>(proposal), static_cast<uint32>(INVALID_PROPOSAL_INDEX));
	ASSERT_TRUE(voteEconomics(PLAYER, proposal));
	callSystemProcedure(PLDT_CONTRACT_INDEX, END_EPOCH);

	auto configuration = makeGame();
	configuration.creatorFeePercent = 99;
	const auto valid = preview(configuration);
	ASSERT_EQ(valid.returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(valid.creatorFee + valid.burn + valid.prizeContribution, configuration.ticketPrice - valid.platformFee);
}

TEST_F(ContractTestingPulseEditorV3, PreviewRejectsAggregateLiabilityOutsideQpiTransferRange)
{
	auto configuration = makeGame(11, 0);
	configuration.ticketPrice = 100000000000000ULL;
	EXPECT_EQ(preview(configuration).returnCode, PLDT::EReturnCode::INVALID_VALUE);
}

TEST_F(ContractTestingPulseEditorV3, PreviewRejectsAmountsOutsideQpiTransferRange)
{
	auto configuration = makeGame(1, 0);
	configuration.ticketPrice = static_cast<uint64>(MAX_AMOUNT) + 1;
	EXPECT_EQ(preview(configuration).returnCode, PLDT::EReturnCode::INVALID_VALUE);

	configuration.ticketPrice = 1;
	configuration.creatorPrizeSeed = static_cast<uint64>(MAX_AMOUNT);
	EXPECT_EQ(preview(configuration).returnCode, PLDT::EReturnCode::INVALID_VALUE);
	configuration.creatorPrizeSeed = static_cast<uint64>(MAX_AMOUNT) - PLDT_DEFAULT_ROUND_FEE;
	EXPECT_EQ(preview(configuration).returnCode, PLDT::EReturnCode::SUCCESS);
}

TEST_F(ContractTestingPulseEditorV3, PreviewRejectsWeightAssignedToTierOutsideCodeLength)
{
	auto configuration = makeGame();
	configuration.tierWeightsBps = {};
	configuration.tierWeightsBps.set(PLDT::payoutMatrixIndex(5, 0), 10000);
	EXPECT_EQ(preview(configuration).returnCode, PLDT::EReturnCode::INVALID_VALUE);
}

TEST_F(ContractTestingPulseEditorV3, PreviewUniqueTiersRequireReachableDigitOverlap)
{
	auto configuration = makeGame();
	configuration.allowRepeatedDigits = false;
	for (const uint8 length : std::initializer_list<uint8>{8, 10})
	{
		SCOPED_TRACE(static_cast<unsigned>(length));
		configuration.codeLength = length;
		configuration.maxDigit = length - 1;
		configuration.tierWeightsBps = {};
		configuration.tierWeightsBps.set(PLDT::payoutMatrixIndex(0, 0), 10000);
		EXPECT_EQ(preview(configuration).returnCode, PLDT::EReturnCode::INVALID_VALUE);
		configuration.tierWeightsBps = {};
		configuration.tierWeightsBps.set(PLDT::payoutMatrixIndex(0, length), 10000);
		EXPECT_EQ(preview(configuration).returnCode, PLDT::EReturnCode::SUCCESS);
	}
	// Two five-element subsets of an eight-digit alphabet must share at least two digits.
	configuration.codeLength = 5;
	configuration.maxDigit = 7;
	configuration.tierWeightsBps = {};
	configuration.tierWeightsBps.set(PLDT::payoutMatrixIndex(0, 1), 10000);
	EXPECT_EQ(preview(configuration).returnCode, PLDT::EReturnCode::INVALID_VALUE);
	configuration.tierWeightsBps = {};
	configuration.tierWeightsBps.set(PLDT::payoutMatrixIndex(0, 2), 10000);
	EXPECT_EQ(preview(configuration).returnCode, PLDT::EReturnCode::SUCCESS);
	configuration.allowRepeatedDigits = true;
	configuration.tierWeightsBps = {};
	configuration.tierWeightsBps.set(PLDT::payoutMatrixIndex(0, 0), 10000);
	EXPECT_EQ(preview(configuration).returnCode, PLDT::EReturnCode::SUCCESS);
	configuration.tierWeightsBps = {};
	configuration.tierWeightsBps.set(PLDT::payoutMatrixIndex(4, 1), 10000);
	EXPECT_EQ(preview(configuration).returnCode, PLDT::EReturnCode::INVALID_VALUE);
}

TEST_F(ContractTestingPulseEditorV3, UnreachableTierCreationOnlyChargesOperationFee)
{
	ASSERT_EQ(createWallet(CREATOR, PLDT_DEFAULT_WALLET_CREATION_FEE + 100).returnCode, PLDT::EReturnCode::SUCCESS);
	const auto* state = reinterpret_cast<const PLDT::StateData*>(contractStates[PLDT_CONTRACT_INDEX]);
	for (const uint8 length : std::initializer_list<uint8>{8, 10})
	{
		SCOPED_TRACE(static_cast<unsigned>(length));
		auto configuration = makeGame(1, 100);
		configuration.codeLength = length;
		configuration.maxDigit = length - 1;
		configuration.allowRepeatedDigits = false;
		configuration.tierWeightsBps = {};
		configuration.tierWeightsBps.set(PLDT::payoutMatrixIndex(0, 0), 10000);
		const auto walletBefore = wallet(CREATOR);
		const auto accountingBefore = platformAccounting();
		const auto balanceBefore = getBalance(contractId());
		const auto created = procedure<PLDT::CreateGame_input, PLDT::CreateGame_output>(PLDT_PROCEDURE_CREATE_GAME, configuration, CREATOR);
		EXPECT_EQ(created.returnCode, PLDT::EReturnCode::INVALID_VALUE);
		EXPECT_EQ(created.gameId, 0ULL);
		EXPECT_EQ(wallet(CREATOR).serviceCredit, walletBefore.serviceCredit - PLDT_DEFAULT_OPERATION_FEE);
		EXPECT_EQ(wallet(CREATOR).refundableQubic, walletBefore.refundableQubic);
		EXPECT_EQ(static_cast<uint32>(wallet(CREATOR).activeGameCount), 0U);
		EXPECT_EQ(getBalance(contractId()), balanceBefore - PLDT_DEFAULT_OPERATION_FEE);
		EXPECT_EQ(static_cast<uint32>(platformAccounting().activeGameCount), 0U);
		EXPECT_EQ(platformAccounting().developer1Accrued, accountingBefore.developer1Accrued);
		EXPECT_EQ(platformAccounting().developer2Accrued, accountingBefore.developer2Accrued);
		EXPECT_EQ(platformAccounting().dividendAccrued, accountingBefore.dividendAccrued);
		EXPECT_EQ(static_cast<uint32>(state->nextUnusedGameSlot), 0U);
		EXPECT_EQ(static_cast<uint32>(state->freeGameHead), 0U);
		EXPECT_EQ(state->generations.get(0), 0ULL);
		EXPECT_EQ(state->games.get(0).status, PLDT::EGameStatus::EMPTY_SLOT);
	}
}

TEST_F(ContractTestingPulseEditorV3, PreviewEnforcesTicketAndBonusAssetCapacityBounds)
{
	auto configuration = makeGame(PLDT_MAX_TICKETS_PER_GAME, 0);
	configuration.playerTicketLimit = PLDT_MAX_TICKETS_PER_GAME;
	EXPECT_EQ(preview(configuration).returnCode, PLDT::EReturnCode::SUCCESS);
	configuration.ticketLimit = PLDT_MAX_TICKETS_PER_GAME + 1;
	configuration.playerTicketLimit = configuration.ticketLimit;
	EXPECT_EQ(preview(configuration).returnCode, PLDT::EReturnCode::INVALID_VALUE);

	const Asset bonus{CREATOR, assetNameFromString("PEDB08")};
	ASSERT_EQ(issueAsset(bonus, 1), 1);
	configuration = makeGame(1, 0);
	configuration.bonusMultiplierBps = 12000;
	configuration.bonusAssetCount = PLDT_MAX_BONUS_ASSETS;
	for (uint16 i = 0; i < PLDT_MAX_BONUS_ASSETS; ++i)
	{
		configuration.bonusAssets.set(i, bonus);
	}
	EXPECT_EQ(preview(configuration).returnCode, PLDT::EReturnCode::SUCCESS);
	configuration.bonusAssetCount = PLDT_MAX_BONUS_ASSETS + 1;
	EXPECT_EQ(preview(configuration).returnCode, PLDT::EReturnCode::INVALID_VALUE);
}

TEST_F(ContractTestingPulseEditorV3, GetGameReportsClosedAtDrawTimeBeforeAutomationVisitsSlot)
{
	fund(CREATOR, 100);
	const auto created = createGame(CREATOR, makeGame(10, 100));
	setCalendar(2025, 1, 4);
	const auto stored = game(created.gameId);
	ASSERT_EQ(stored.returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(stored.game.status, PLDT::EGameStatus::CLOSED);
}

TEST_F(ContractTestingPulseEditorV3, SettlementResetsMatchCountsForEveryTicket)
{
	auto configuration = makeGame(4, 100);
	configuration.codeLength = PLDT_MIN_CODE_LENGTH;
	configuration.maxDigit = PLDT_MIN_MAX_DIGIT;
	configuration.allowRepeatedDigits = true;
	configuration.tierWeightsBps = {};
	configuration.tierWeightsBps.set(PLDT::payoutMatrixIndex(0, 0), 10000);
	fund(CREATOR, 100);
	const auto created = createGame(CREATOR, configuration);
	setCalendar(2025, 1, 3);
	fund(PLAYER, 200);
	fund(SECOND_PLAYER, 200);

	PLDT::BuyTickets_input firstBatch{};
	firstBatch.gameId = created.gameId;
	firstBatch.ticketCount = 2;
	firstBatch.tickets.set(0, {digits(0)});
	firstBatch.tickets.set(1, {digits(0)});
	auto mixed = digits(0);
	mixed.set(1, 1);
	firstBatch.tickets.set(1, {mixed});
	const auto first = procedure<PLDT::BuyTickets_input, PLDT::BuyTickets_output>(PLDT_PROCEDURE_BUY_TICKETS, firstBatch, PLAYER, 200);

	PLDT::BuyTickets_input secondBatch{};
	secondBatch.gameId = created.gameId;
	secondBatch.ticketCount = 2;
	mixed = digits(1);
	mixed.set(1, 0);
	secondBatch.tickets.set(0, {mixed});
	mixed = digits(1);
	mixed.set(1, 1);
	secondBatch.tickets.set(1, {mixed});
	const auto second = procedure<PLDT::BuyTickets_input, PLDT::BuyTickets_output>(PLDT_PROCEDURE_BUY_TICKETS, secondBatch, SECOND_PLAYER, 200);
	ASSERT_EQ(static_cast<uint32>(first.acceptedCount), 2U);
	ASSERT_EQ(static_cast<uint32>(second.acceptedCount), 2U);
	beginTickAt(100);

	for (uint16 i = 0; i < 2; ++i)
	{
		const auto firstTicket = ticket(first.ticketIndexes.get(i)).ticket;
		const auto secondTicket = ticket(second.ticketIndexes.get(i)).ticket;
		EXPECT_LE(firstTicket.exact + firstTicket.misplaced, PLDT_MIN_CODE_LENGTH);
		EXPECT_LE(secondTicket.exact + secondTicket.misplaced, PLDT_MIN_CODE_LENGTH);
		EXPECT_FALSE(firstTicket.exact == 3 && firstTicket.misplaced == 1);
		EXPECT_FALSE(secondTicket.exact == 3 && secondTicket.misplaced == 1);
	}
}

TEST_F(ContractTestingPulseEditorV3, FailedWinnerTransferRetriesWithoutConsumingTierRemainder)
{
	fund(CREATOR, 101);
	const auto configuration = makeGame(2, 101);
	const auto created = createGame(CREATOR, configuration);
	const m256i digest(37, 38, 39, 40);
	etalonTick.prevSpectrumDigest = digest;
	const auto winning =
	    expectedWinningDigits(digest, created.gameId, 2, configuration.codeLength, configuration.maxDigit, configuration.allowRepeatedDigits);
	setCalendar(2025, 1, 3);
	fund(PLAYER, 100);
	fund(SECOND_PLAYER, 100);
	const auto first = buyDigits(PLAYER, created.gameId, winning);
	const auto second = buyDigits(SECOND_PLAYER, created.gameId, winning);

	const auto selfIndex = spectrumIndex(contractId());
	ASSERT_GE(selfIndex, 0);
	ASSERT_TRUE(decreaseEnergy(selfIndex, getBalance(contractId())));
	beginTickAt(100);
	EXPECT_EQ(ticket(first.ticketIndexes.get(0)).ticket.status, PLDT::ETicketStatus::ACTIVE);

	increaseEnergy(contractId(), 295);
	processFirstGameAt(200);
	const auto finished = result(created.gameId);
	ASSERT_EQ(finished.returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(finished.gameResult.totalPaid, 295);
	EXPECT_EQ(ticket(first.ticketIndexes.get(0)).ticket.payout, 148);
	EXPECT_EQ(ticket(second.ticketIndexes.get(0)).ticket.payout, 147);
}

TEST_F(ContractTestingPulseEditorV3, InvalidBatchLeavesPaymentAndGameStateUnchanged)
{
	fund(CREATOR, 100);
	const auto created = createGame(CREATOR, makeGame(2, 100));
	setCalendar(2025, 1, 3);
	fund(PLAYER, 200);
	const auto balanceBefore = getBalance(PLAYER);
	PLDT::BuyTickets_input input{};
	input.gameId = created.gameId;
	input.ticketCount = 2;
	input.tickets.set(0, {digits(0)});
	auto invalidDigits = digits(0);
	invalidDigits.set(0, PLDT_MIN_MAX_DIGIT + 1);
	input.tickets.set(1, {invalidDigits});
	const auto purchase = procedure<PLDT::BuyTickets_input, PLDT::BuyTickets_output>(PLDT_PROCEDURE_BUY_TICKETS, input, PLAYER, 200);

	EXPECT_EQ(purchase.returnCode, PLDT::EReturnCode::INVALID_DIGITS);
	EXPECT_EQ(static_cast<uint32>(purchase.acceptedCount), 0U);
	EXPECT_EQ(getBalance(PLAYER), balanceBefore);
	const auto stored = game(created.gameId).game;
	EXPECT_EQ(static_cast<uint32>(stored.ticketCount), 0U);
	EXPECT_EQ(stored.prizePool, 100);
	EXPECT_EQ(stored.totalRevenue, 0);
}

TEST_F(ContractTestingPulseEditorV3, BatchReportsInvalidGameAndRefundsPayment)
{
	fund(PLAYER, 100);
	const auto balanceBefore = getBalance(PLAYER);
	PLDT::BuyTickets_input input{};
	input.gameId = 999999;
	input.ticketCount = 1;
	input.tickets.set(0, {digits(0)});
	const auto purchase = procedure<PLDT::BuyTickets_input, PLDT::BuyTickets_output>(PLDT_PROCEDURE_BUY_TICKETS, input, PLAYER, 100);
	EXPECT_EQ(purchase.returnCode, PLDT::EReturnCode::INVALID_GAME);
	EXPECT_EQ(static_cast<uint32>(purchase.acceptedCount), 0U);
	EXPECT_EQ(getBalance(PLAYER), balanceBefore);
}

TEST_F(ContractTestingPulseEditorV3, InvalidSingleTicketPriceIsFullyRefunded)
{
	fund(CREATOR, 100);
	const auto created = createGame(CREATOR, makeGame(2, 100));
	setCalendar(2025, 1, 3);
	fund(PLAYER, 101);
	const auto balanceBefore = getBalance(PLAYER);
	const auto purchase = buyAtPrice(PLAYER, created.gameId, 0, 101);
	EXPECT_EQ(purchase.returnCode, PLDT::EReturnCode::TICKET_INVALID_PRICE);
	EXPECT_EQ(getBalance(PLAYER), balanceBefore);
	EXPECT_EQ(static_cast<uint32>(game(created.gameId).game.ticketCount), 0U);
}

TEST_F(ContractTestingPulseEditorV3, AssetGameUsesSameEconomicsAndPaysEntirePool)
{
	const Asset currency{CREATOR, assetNameFromString("PEDAST")};
	ASSERT_EQ(issueAsset(currency, 10000), 10000);
	ASSERT_EQ(transferAsset(currency, CREATOR, PLAYER, 100), 100);
	ASSERT_EQ(transferAssetManagement(currency, CREATOR, 100, PLDT_CONTRACT_INDEX), 100);
	ASSERT_EQ(transferAssetManagement(currency, PLAYER, 100, PLDT_CONTRACT_INDEX), 100);

	auto configuration = makeGame(1, 100);
	configuration.currencyMode = PLDT::ECurrencyMode::ASSET;
	configuration.currencyAsset = currency;
	configuration.ownershipManagingContractIndex = PLDT_CONTRACT_INDEX;
	configuration.possessionManagingContractIndex = PLDT_CONTRACT_INDEX;
	const auto created = createGame(CREATOR, configuration);
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
	setCalendar(2025, 1, 3);
	const auto purchase = buyAsset(PLAYER, created.gameId, 0);
	ASSERT_EQ(purchase.returnCode, PLDT::EReturnCode::SUCCESS);
	beginTickAt(100);

	const auto finished = result(created.gameId);
	ASSERT_EQ(finished.returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(finished.gameResult.prizePool, 197);
	EXPECT_EQ(finished.gameResult.totalPaid, 197);
	EXPECT_EQ(numberOfPossessedShares(currency.assetName, currency.issuer, PLAYER, PLAYER, PLDT_CONTRACT_INDEX, PLDT_CONTRACT_INDEX), 197);
	const auto playerWallet = wallet(PLAYER);
	ASSERT_TRUE(playerWallet.found);
	ASSERT_EQ(playerWallet.assetCount, 1);
	EXPECT_EQ(playerWallet.assets.get(0).asset.assetName, currency.assetName);
	EXPECT_EQ(playerWallet.assets.get(0).asset.issuer, currency.issuer);
	EXPECT_EQ(playerWallet.assets.get(0).balance, 197);
}

TEST_F(ContractTestingPulseEditorV3, OpenWalletCanReleaseLegacyUntrackedManagedShares)
{
	const Asset asset{CREATOR, assetNameFromString("PEDLEG")};
	ASSERT_EQ(issueAsset(asset, 10), 10);
	ASSERT_EQ(transferAssetManagement(asset, CREATOR, 10, PLDT_CONTRACT_INDEX), 10);

	auto* contractState = reinterpret_cast<PLDT::StateData*>(contractStates[PLDT_CONTRACT_INDEX]);
	PLDT::CreatorWallet storedWallet{};
	ASSERT_TRUE(contractState->wallets.get(CREATOR, storedWallet));
	PLDT::WalletAssetBalance emptyAsset{};
	storedWallet.assets.set(0, emptyAsset);
	storedWallet.assetCount = 0;
	ASSERT_TRUE(contractState->wallets.replace(CREATOR, storedWallet));

	fund(CREATOR, 100);
	const auto released = releaseAssetManagement(CREATOR, asset, 10);
	EXPECT_EQ(released.returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(numberOfPossessedShares(asset.assetName, asset.issuer, CREATOR, CREATOR, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX), 10);
}

TEST_F(ContractTestingPulseEditorV3, FullWalletWinnerCanReleaseUntrackedPayout)
{
	const Asset currency{CREATOR, assetNameFromString("PEDFUL")};
	ASSERT_EQ(issueAsset(currency, 10000), 10000);
	ASSERT_EQ(transferAsset(currency, CREATOR, PLAYER, 100), 100);
	ASSERT_EQ(transferAssetManagement(currency, CREATOR, 100, PLDT_CONTRACT_INDEX), 100);
	ASSERT_EQ(transferAssetManagement(currency, PLAYER, 100, PLDT_CONTRACT_INDEX), 100);

	auto configuration = makeGame(1, 100);
	configuration.currencyMode = PLDT::ECurrencyMode::ASSET;
	configuration.currencyAsset = currency;
	configuration.ownershipManagingContractIndex = PLDT_CONTRACT_INDEX;
	configuration.possessionManagingContractIndex = PLDT_CONTRACT_INDEX;
	const auto created = createGame(CREATOR, configuration);
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
	setCalendar(2025, 1, 3);
	ASSERT_EQ(buyAsset(PLAYER, created.gameId, 0).returnCode, PLDT::EReturnCode::SUCCESS);

	auto* contractState = reinterpret_cast<PLDT::StateData*>(contractStates[PLDT_CONTRACT_INDEX]);
	PLDT::CreatorWallet fullWallet{};
	ASSERT_TRUE(contractState->wallets.get(PLAYER, fullWallet));
	for (uint8 slot = 0; slot < PLDT_MAX_WALLET_ASSETS; ++slot)
	{
		PLDT::WalletAssetBalance filler{};
		filler.asset = Asset{PLAYER, static_cast<uint64>(slot + 1)};
		filler.balance = 1;
		filler.isActive = true;
		fullWallet.assets.set(slot, filler);
	}
	fullWallet.assetCount = PLDT_MAX_WALLET_ASSETS;
	ASSERT_TRUE(contractState->wallets.replace(PLAYER, fullWallet));

	beginTickAt(100);
	ASSERT_EQ(result(created.gameId).returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(wallet(PLAYER).assetCount, PLDT_MAX_WALLET_ASSETS);
	EXPECT_EQ(numberOfPossessedShares(currency.assetName, currency.issuer, PLAYER, PLAYER, PLDT_CONTRACT_INDEX, PLDT_CONTRACT_INDEX), 197);
	fund(PLAYER, 100);
	EXPECT_EQ(releaseAssetManagement(PLAYER, currency, 197).returnCode, PLDT::EReturnCode::SUCCESS);
}

TEST_F(ContractTestingPulseEditorV3, NonPayableProceduresRefundUnexpectedInvocationReward)
{
	auto permanentGame = makeGame(2, 100);
	permanentGame.mode = PLDT::EGameMode::PERMANENT;
	const auto created = createGame(CREATOR, permanentGame);
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
	fund(CREATOR, 200);
	const auto creatorBalanceBefore = getBalance(CREATOR);

	PLDT::UpdateGameEconomics_input economicsInput{};
	economicsInput.gameId = created.gameId;
	economicsInput.ticketPrice = permanentGame.ticketPrice;
	economicsInput.creatorPrizeSeed = permanentGame.creatorPrizeSeed;
	economicsInput.ticketLimit = permanentGame.ticketLimit;
	economicsInput.playerTicketLimit = permanentGame.playerTicketLimit;
	economicsInput.creatorFeePercent = permanentGame.creatorFeePercent;
	EXPECT_EQ((procedure<PLDT::UpdateGameEconomics_input, PLDT::UpdateGameEconomics_output>(PLDT_PROCEDURE_UPDATE_GAME_ECONOMICS, economicsInput,
	                                                                                        CREATOR, 100))
	              .returnCode,
	          PLDT::EReturnCode::INVALID_VALUE);
	EXPECT_EQ(getBalance(CREATOR), creatorBalanceBefore);
	EXPECT_FALSE(game(created.gameId).game.pendingEconomics.isSet);

	PLDT::StopGame_input stopInput{};
	stopInput.gameId = created.gameId;
	EXPECT_EQ((procedure<PLDT::StopGame_input, PLDT::StopGame_output>(PLDT_PROCEDURE_STOP_GAME, stopInput, CREATOR, 100)).returnCode,
	          PLDT::EReturnCode::INVALID_VALUE);
	EXPECT_EQ(getBalance(CREATOR), creatorBalanceBefore);
	EXPECT_EQ(game(created.gameId).returnCode, PLDT::EReturnCode::SUCCESS);

	fund(PULSE_TEAM_OWNER, 500);
	const auto balanceBefore = getBalance(PULSE_TEAM_OWNER);

	const auto accounting = platformAccounting();
	PLDT::SetPlatformConfig_input configInput{};
	configInput.platformOwner = accounting.platformOwner;
	configInput.developer1 = DEVELOPER1;
	configInput.developer2 = DEVELOPER2;
	EXPECT_EQ((procedure<PLDT::SetPlatformConfig_input, PLDT::SetPlatformConfig_output>(PLDT_PROCEDURE_SET_PLATFORM_CONFIG, configInput,
	                                                                                    PULSE_TEAM_OWNER, 100))
	              .returnCode,
	          PLDT::EReturnCode::INVALID_VALUE);
	EXPECT_EQ(getBalance(PULSE_TEAM_OWNER), balanceBefore);

	PLDT::WithdrawPlatformRevenue_input withdrawInput{};
	EXPECT_EQ(
	    (procedure<PLDT::WithdrawPlatformRevenue_input, PLDT::WithdrawPlatformRevenue_output>(9, withdrawInput, PULSE_TEAM_OWNER, 100)).returnCode,
	    PLDT::EReturnCode::INVALID_VALUE);
	EXPECT_EQ(getBalance(PULSE_TEAM_OWNER), balanceBefore);

	PLDT::WithdrawAssetPlatformRevenue_input assetWithdrawInput{};
	EXPECT_EQ((procedure<PLDT::WithdrawAssetPlatformRevenue_input, PLDT::WithdrawAssetPlatformRevenue_output>(16, assetWithdrawInput,
	                                                                                                          PULSE_TEAM_OWNER, 100))
	              .returnCode,
	          PLDT::EReturnCode::INVALID_VALUE);
	EXPECT_EQ(getBalance(PULSE_TEAM_OWNER), balanceBefore);
}

TEST_F(ContractTestingPulseEditorV3, AssetCancellationWaitsForWalletPositionBeforeReturningSeed)
{
	const Asset currency{CREATOR, assetNameFromString("PEDCAN")};
	ASSERT_EQ(issueAsset(currency, 100), 100);
	ASSERT_EQ(transferAssetManagement(currency, CREATOR, 100, PLDT_CONTRACT_INDEX), 100);
	auto configuration = makeGame(1, 100);
	configuration.currencyMode = PLDT::ECurrencyMode::ASSET;
	configuration.currencyAsset = currency;
	configuration.ownershipManagingContractIndex = PLDT_CONTRACT_INDEX;
	configuration.possessionManagingContractIndex = PLDT_CONTRACT_INDEX;
	const auto created = createGame(CREATOR, configuration);
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
	auto* contractState = reinterpret_cast<PLDT::StateData*>(contractStates[PLDT_CONTRACT_INDEX]);
	PLDT::CreatorWallet fullWallet{};
	ASSERT_TRUE(contractState->wallets.get(CREATOR, fullWallet));
	const auto pinnedPosition = fullWallet.assets.get(0);
	for (uint8 slot = 0; slot < PLDT_MAX_WALLET_ASSETS; ++slot)
	{
		PLDT::WalletAssetBalance filler{};
		filler.asset = Asset{CREATOR, static_cast<uint64>(slot + 100)};
		filler.balance = 1;
		filler.isActive = true;
		fullWallet.assets.set(slot, filler);
	}
	fullWallet.assetCount = PLDT_MAX_WALLET_ASSETS;
	ASSERT_TRUE(contractState->wallets.replace(CREATOR, fullWallet));
	ASSERT_EQ(stop(CREATOR, created.gameId).returnCode, PLDT::EReturnCode::STORAGE_FULL);
	EXPECT_EQ(game(created.gameId).game.pendingPrizePoolPayout, 100ULL);
	EXPECT_EQ(numberOfPossessedShares(currency.assetName, currency.issuer, CREATOR, CREATOR, PLDT_CONTRACT_INDEX, PLDT_CONTRACT_INDEX), 0);
	ASSERT_TRUE(contractState->wallets.get(CREATOR, fullWallet));
	fullWallet.assets.set(0, pinnedPosition);
	ASSERT_TRUE(contractState->wallets.replace(CREATOR, fullWallet));
	processFirstGameAt(100);
	EXPECT_EQ(result(created.gameId).gameResult.terminalReason, PLDT::EGameTerminalReason::OWNER_CANCELLED);
	EXPECT_EQ(numberOfPossessedShares(currency.assetName, currency.issuer, CREATOR, CREATOR, PLDT_CONTRACT_INDEX, PLDT_CONTRACT_INDEX), 100);
	EXPECT_EQ(wallet(CREATOR).assetCount, PLDT_MAX_WALLET_ASSETS);
	EXPECT_EQ(wallet(CREATOR).assets.get(0).balance, 100ULL);
	fund(CREATOR, 100);
	EXPECT_EQ(releaseAssetManagement(CREATOR, currency, 100).returnCode, PLDT::EReturnCode::SUCCESS);
}

TEST_F(ContractTestingPulseEditorV3, AssetBurnFailureCompensatesInTheTicketCurrency)
{
	const Asset configuredCurrency{CREATOR, assetNameFromString("PEDFAIL")};
	ASSERT_EQ(issueAsset(configuredCurrency, 103), 103);
	ASSERT_EQ(transferAsset(configuredCurrency, CREATOR, PLAYER, 103), 103);
	ASSERT_EQ(transferAssetManagement(configuredCurrency, PLAYER, 103, PLDT_CONTRACT_INDEX), 103);
	auto configuration = makeGame(1, 0);
	configuration.ticketPrice = 103;
	configuration.currencyMode = PLDT::ECurrencyMode::ASSET;
	configuration.currencyAsset = configuredCurrency;
	configuration.ownershipManagingContractIndex = PLDT_CONTRACT_INDEX;
	configuration.possessionManagingContractIndex = PLDT_CONTRACT_INDEX;
	const auto created = createGame(CREATOR, configuration);
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);

	// Contract shares cannot be burned. Replacing only the stored transfer currency injects
	// that deterministic failure after validation without exposing an unplayable public game.
	issuePulseEditorSharesTo(PLAYER);
	const Asset contractShares{NULL_ID, assetNameFromString("PLDT")};
	ASSERT_EQ(transferAssetManagement(contractShares, PLAYER, NUMBER_OF_COMPUTORS, PLDT_CONTRACT_INDEX), NUMBER_OF_COMPUTORS);
	auto* contractState = reinterpret_cast<PLDT::StateData*>(contractStates[PLDT_CONTRACT_INDEX]);
	auto storedGame = contractState->games.get(created.slot);
	storedGame.currencyAsset = contractShares;
	contractState->games.set(created.slot, storedGame);
	setCalendar(2025, 1, 3);

	const auto purchase = buyAsset(PLAYER, created.gameId, 0);
	EXPECT_EQ(purchase.returnCode, PLDT::EReturnCode::TRANSFER_FAILED);
	EXPECT_EQ(static_cast<uint32>(game(created.gameId).game.ticketCount), 0U);
	EXPECT_EQ(numberOfPossessedShares(contractShares.assetName, contractShares.issuer, PLAYER, PLAYER, PLDT_CONTRACT_INDEX, PLDT_CONTRACT_INDEX),
	          NUMBER_OF_COMPUTORS);
	EXPECT_EQ(numberOfPossessedShares(contractShares.assetName, contractShares.issuer, contractId(), contractId(), PLDT_CONTRACT_INDEX,
	                                  PLDT_CONTRACT_INDEX),
	          0);
}

TEST_F(ContractTestingPulseEditorV3, PreviewRejectsContractShareCurrencyWhenTicketBurnIsPositive)
{
	issuePulseEditorSharesTo(PLAYER);
	const Asset contractShares{NULL_ID, assetNameFromString("PLDT")};
	ASSERT_EQ(transferAssetManagement(contractShares, PLAYER, NUMBER_OF_COMPUTORS, PLDT_CONTRACT_INDEX), NUMBER_OF_COMPUTORS);
	auto configuration = makeGame(1, 0);
	configuration.currencyMode = PLDT::ECurrencyMode::ASSET;
	configuration.currencyAsset = contractShares;
	configuration.ownershipManagingContractIndex = PLDT_CONTRACT_INDEX;
	configuration.possessionManagingContractIndex = PLDT_CONTRACT_INDEX;

	configuration.ticketPrice = 102;
	EXPECT_EQ(preview(configuration).returnCode, PLDT::EReturnCode::SUCCESS);
	configuration.ticketPrice = 103;
	EXPECT_EQ(preview(configuration).returnCode, PLDT::EReturnCode::INVALID_VALUE);
}

TEST_F(ContractTestingPulseEditorV3, CancelledZeroSeedAssetGameReleasesUnusedAccountingBucket)
{
	const Asset currency{CREATOR, assetNameFromString("PEDIDLE")};
	ASSERT_EQ(issueAsset(currency, 1), 1);
	ASSERT_EQ(transferAssetManagement(currency, CREATOR, 1, PLDT_CONTRACT_INDEX), 1);
	auto configuration = makeGame(1, 0);
	configuration.currencyMode = PLDT::ECurrencyMode::ASSET;
	configuration.currencyAsset = currency;
	configuration.ownershipManagingContractIndex = PLDT_CONTRACT_INDEX;
	configuration.possessionManagingContractIndex = PLDT_CONTRACT_INDEX;
	const auto created = createGame(CREATOR, configuration);
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
	ASSERT_EQ(stop(CREATOR, created.gameId).returnCode, PLDT::EReturnCode::SUCCESS);

	const auto* contractState = reinterpret_cast<const PLDT::StateData*>(contractStates[PLDT_CONTRACT_INDEX]);
	for (uint16 i = 0; i < contractState->assetAccounting.capacity(); ++i)
	{
		EXPECT_FALSE(contractState->assetAccounting.get(i).isActive);
	}
}

TEST_F(ContractTestingPulseEditorV3, PreviewRejectsUnknownOrExternallyManagedCurrencyAsset)
{
	auto configuration = makeGame(1, 0);
	configuration.currencyMode = PLDT::ECurrencyMode::ASSET;
	configuration.currencyAsset = Asset{CREATOR, assetNameFromString("MISSING")};
	configuration.ownershipManagingContractIndex = PLDT_CONTRACT_INDEX;
	configuration.possessionManagingContractIndex = PLDT_CONTRACT_INDEX;
	EXPECT_EQ(preview(configuration).returnCode, PLDT::EReturnCode::INVALID_VALUE);

	ASSERT_EQ(issueAsset(configuration.currencyAsset, 1), 1);
	configuration.ownershipManagingContractIndex = PLDT_CONTRACT_INDEX;
	configuration.possessionManagingContractIndex = PLDT_CONTRACT_INDEX;
	EXPECT_EQ(preview(configuration).returnCode, PLDT::EReturnCode::INVALID_VALUE);

	configuration.ownershipManagingContractIndex = QX_CONTRACT_INDEX;
	configuration.possessionManagingContractIndex = QX_CONTRACT_INDEX;
	EXPECT_EQ(preview(configuration).returnCode, PLDT::EReturnCode::INVALID_VALUE);
}

TEST_F(ContractTestingPulseEditorV3, MultipleQualifyingAssetsDoNotStackOwnershipMultiplier)
{
	const Asset firstBonus{CREATOR, assetNameFromString("PEDB01")};
	const Asset secondBonus{CREATOR, assetNameFromString("PEDB02")};
	ASSERT_EQ(issueAsset(firstBonus, 10), 10);
	ASSERT_EQ(issueAsset(secondBonus, 10), 10);
	ASSERT_EQ(transferAsset(firstBonus, CREATOR, PLAYER, 1), 1);
	ASSERT_EQ(transferAsset(secondBonus, CREATOR, PLAYER, 1), 1);

	auto configuration = makeGame(2, 100);
	configuration.bonusAssets.set(0, firstBonus);
	configuration.bonusAssets.set(1, secondBonus);
	configuration.bonusAssetCount = 2;
	configuration.bonusMultiplierBps = 20000;
	configuration.bonusOwnershipManagingContractIndex = QX_CONTRACT_INDEX;
	configuration.bonusPossessionManagingContractIndex = QX_CONTRACT_INDEX;
	fund(CREATOR, 100);
	const auto created = createGame(CREATOR, configuration);
	const m256i digest(41, 42, 43, 44);
	etalonTick.prevSpectrumDigest = digest;
	const auto winning =
	    expectedWinningDigits(digest, created.gameId, 2, configuration.codeLength, configuration.maxDigit, configuration.allowRepeatedDigits);
	setCalendar(2025, 1, 3);
	fund(PLAYER, 100);
	fund(SECOND_PLAYER, 100);
	const auto qualified = buyDigits(PLAYER, created.gameId, winning);
	const auto regular = buyDigits(SECOND_PLAYER, created.gameId, winning);
	beginTickAt(100);

	EXPECT_TRUE(ticket(qualified.ticketIndexes.get(0)).ticket.bonusQualified);
	EXPECT_FALSE(ticket(regular.ticketIndexes.get(0)).ticket.bonusQualified);
	EXPECT_EQ(ticket(qualified.ticketIndexes.get(0)).ticket.payout, 196);
	EXPECT_EQ(ticket(regular.ticketIndexes.get(0)).ticket.payout, 98);
	EXPECT_EQ(result(created.gameId).gameResult.totalPaid, 294);
}

TEST_F(ContractTestingPulseEditorV3, BonusQualificationIsSnapshottedWhenTicketIsBought)
{
	const Asset bonus{CREATOR, assetNameFromString("PEDSNAP")};
	ASSERT_EQ(issueAsset(bonus, 1), 1);
	ASSERT_EQ(transferAsset(bonus, CREATOR, PLAYER, 1), 1);

	auto configuration = makeGame(2, 100);
	configuration.bonusAssets.set(0, bonus);
	configuration.bonusAssetCount = 1;
	configuration.bonusMultiplierBps = 20000;
	configuration.bonusOwnershipManagingContractIndex = QX_CONTRACT_INDEX;
	configuration.bonusPossessionManagingContractIndex = QX_CONTRACT_INDEX;
	fund(CREATOR, 100);
	const auto created = createGame(CREATOR, configuration);
	const m256i digest(45, 46, 47, 48);
	etalonTick.prevSpectrumDigest = digest;
	const auto winning =
	    expectedWinningDigits(digest, created.gameId, 2, configuration.codeLength, configuration.maxDigit, configuration.allowRepeatedDigits);
	setCalendar(2025, 1, 3);
	fund(PLAYER, 100);
	fund(SECOND_PLAYER, 100);
	const auto qualifiedAtPurchase = buyDigits(PLAYER, created.gameId, winning);
	const auto regularAtPurchase = buyDigits(SECOND_PLAYER, created.gameId, winning);
	ASSERT_EQ(transferAsset(bonus, PLAYER, SECOND_PLAYER, 1), 1);
	beginTickAt(100);

	EXPECT_TRUE(ticket(qualifiedAtPurchase.ticketIndexes.get(0)).ticket.bonusQualified);
	EXPECT_FALSE(ticket(regularAtPurchase.ticketIndexes.get(0)).ticket.bonusQualified);
	EXPECT_EQ(ticket(qualifiedAtPurchase.ticketIndexes.get(0)).ticket.payout, 196);
	EXPECT_EQ(ticket(regularAtPurchase.ticketIndexes.get(0)).ticket.payout, 98);
}

TEST_F(ContractTestingPulseEditorV3, AssetTicketSnapshotsQualificationBeforeCollectingTheEntryAsset)
{
	const Asset currency{CREATOR, assetNameFromString("PEDSELF")};
	ASSERT_EQ(issueAsset(currency, 100), 100);
	ASSERT_EQ(transferAsset(currency, CREATOR, PLAYER, 100), 100);
	ASSERT_EQ(transferAssetManagement(currency, PLAYER, 100, PLDT_CONTRACT_INDEX), 100);
	auto configuration = makeGame(1, 0);
	configuration.currencyMode = PLDT::ECurrencyMode::ASSET;
	configuration.currencyAsset = currency;
	configuration.ownershipManagingContractIndex = PLDT_CONTRACT_INDEX;
	configuration.possessionManagingContractIndex = PLDT_CONTRACT_INDEX;
	configuration.bonusAssets.set(0, currency);
	configuration.bonusAssetCount = 1;
	configuration.bonusMultiplierBps = 20000;
	configuration.bonusOwnershipManagingContractIndex = PLDT_CONTRACT_INDEX;
	configuration.bonusPossessionManagingContractIndex = PLDT_CONTRACT_INDEX;
	const auto created = createGame(CREATOR, configuration);
	setCalendar(2025, 1, 3);

	const auto purchase = buyAsset(PLAYER, created.gameId, 0);

	ASSERT_EQ(purchase.returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_TRUE(ticket(purchase.ticketIndexes.get(0)).ticket.bonusQualified);
	EXPECT_EQ(numberOfPossessedShares(currency.assetName, currency.issuer, PLAYER, PLAYER, PLDT_CONTRACT_INDEX, PLDT_CONTRACT_INDEX), 0);
}

TEST_F(ContractTestingPulseEditorV3, NoWinnerDrawRefundsPoolAndCreatorRevenueAndMarksTicketLost)
{
	auto configuration = makeGame(1, 100);
	configuration.tierWeightsBps = {};
	configuration.tierWeightsBps.set(PLDT::payoutMatrixIndex(4, 0), 10000);
	configuration.maxDigit = PLDT_MIN_MAX_DIGIT;
	configuration.creatorFeePercent = 10;
	fund(CREATOR, 100);
	const auto created = createGame(CREATOR, configuration);
	const m256i digest(13, 14, 15, 16);
	etalonTick.prevSpectrumDigest = digest;
	const auto winning =
	    expectedWinningDigits(digest, created.gameId, 1, configuration.codeLength, configuration.maxDigit, configuration.allowRepeatedDigits);
	const auto losing = nonMatchingDigits(winning, configuration.codeLength, configuration.maxDigit);
	setCalendar(2025, 1, 3);
	fund(PLAYER, 100);
	const auto purchase = buyDigits(PLAYER, created.gameId, losing);
	const auto creatorBalanceBeforeSettlement = getBalance(CREATOR);
	const auto walletBeforeSettlement = wallet(CREATOR);
	beginTickAt(100);

	const auto finished = result(created.gameId);
	ASSERT_EQ(finished.returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(finished.gameResult.terminalReason, PLDT::EGameTerminalReason::NO_WINNERS);
	EXPECT_EQ(finished.gameResult.prizePool, 188);
	EXPECT_EQ(finished.gameResult.totalPaid, 0);
	EXPECT_EQ(ticket(purchase.ticketIndexes.get(0)).ticket.status, PLDT::ETicketStatus::LOST);
	EXPECT_EQ(getBalance(CREATOR), creatorBalanceBeforeSettlement);
	EXPECT_EQ(wallet(CREATOR).serviceCredit + wallet(CREATOR).refundableQubic,
	          walletBeforeSettlement.serviceCredit + walletBeforeSettlement.refundableQubic + 197);
}

TEST_F(ContractTestingPulseEditorV3, GeneratedDigitsStayUniqueAndInsideConfiguredRange)
{
	auto configuration = makeGame(1, 0);
	configuration.tierWeightsBps = {};
	configuration.tierWeightsBps.set(PLDT::payoutMatrixIndex(10, 0), 10000);
	configuration.codeLength = 10;
	configuration.maxDigit = 9;
	configuration.allowRepeatedDigits = false;
	const auto created = createGame(CREATOR, configuration);
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
	setCalendar(2025, 1, 3);
	fund(PLAYER, 100);
	PLDT::BuyTickets_input input{};
	input.gameId = created.gameId;
	input.ticketCount = 1;
	Array<uint8, PLDT_DIGITS_ALIGNED> submittedDigits{};
	for (uint16 i = 0; i < configuration.codeLength; ++i)
	{
		submittedDigits.set(i, static_cast<uint8>(i));
	}
	input.tickets.set(0, {submittedDigits});
	const auto purchase = procedure<PLDT::BuyTickets_input, PLDT::BuyTickets_output>(PLDT_PROCEDURE_BUY_TICKETS, input, PLAYER, 100);
	ASSERT_EQ(purchase.returnCode, PLDT::EReturnCode::SUCCESS);
	const m256i digest(0x123456789abcdef0ULL, 0x0fedcba987654321ULL, 0x1111222233334444ULL, 0xaaaabbbbccccddddULL);
	etalonTick.prevSpectrumDigest = digest;
	beginTickAt(100);

	const auto winningDigits = result(created.gameId).gameResult.winningDigits;
	const auto expected =
	    expectedWinningDigits(digest, created.gameId, 1, configuration.codeLength, configuration.maxDigit, configuration.allowRepeatedDigits);
	bool seen[10]{};
	for (uint16 i = 0; i < configuration.codeLength; ++i)
	{
		EXPECT_EQ(winningDigits.get(i), expected.get(i));
		ASSERT_LT(winningDigits.get(i), 10);
		EXPECT_FALSE(seen[winningDigits.get(i)]);
		seen[winningDigits.get(i)] = true;
	}
}

TEST_F(ContractTestingPulseEditorV3, GetPlayersDeduplicatesHashCollisionsInFirstTicketOrder)
{
	const id firstPlayer{101, 202, 303, 404};
	const id collidingPlayer{202, 101, 303, 404};
	const auto created = createGame(CREATOR, makeGame(4, 0));
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
	setCalendar(2025, 1, 3);
	fund(firstPlayer, 200);
	fund(collidingPlayer, 100);
	fund(OUTSIDER, 100);
	ASSERT_EQ(buy(firstPlayer, created.gameId, 0).returnCode, PLDT::EReturnCode::SUCCESS);
	ASSERT_EQ(buy(collidingPlayer, created.gameId, 1).returnCode, PLDT::EReturnCode::SUCCESS);
	ASSERT_EQ(buy(firstPlayer, created.gameId, 2).returnCode, PLDT::EReturnCode::SUCCESS);
	ASSERT_EQ(buy(OUTSIDER, created.gameId, 3).returnCode, PLDT::EReturnCode::SUCCESS);

	const auto page = players(created.gameId);

	ASSERT_EQ(page.returnCode, PLDT::EReturnCode::SUCCESS);
	ASSERT_EQ(page.totalCount, 3ULL);
	ASSERT_EQ(static_cast<uint32>(page.returnedCount), 3U);
	EXPECT_EQ(page.players.get(0).player, firstPlayer);
	EXPECT_EQ(static_cast<uint32>(page.players.get(0).ticketCount), 2U);
	EXPECT_EQ(page.players.get(1).player, collidingPlayer);
	EXPECT_EQ(static_cast<uint32>(page.players.get(1).ticketCount), 1U);
	EXPECT_EQ(page.players.get(2).player, OUTSIDER);
	EXPECT_EQ(static_cast<uint32>(page.players.get(2).ticketCount), 1U);
	EXPECT_EQ(page.players.get(3).player, NULL_ID);
	EXPECT_EQ(page.players.get(3).totalPayout, 0ULL);
	EXPECT_EQ(static_cast<uint32>(page.players.get(3).ticketCount), 0U);
	EXPECT_EQ(static_cast<uint32>(page.players.get(3).winningTicketCount), 0U);
	EXPECT_EQ(static_cast<uint32>(page.players.get(3).paidTicketCount), 0U);
	EXPECT_EQ(static_cast<uint32>(page.players.get(3).bonusQualifiedTicketCount), 0U);
	for (uint16 i = 0; i < page.padding0.capacity(); ++i)
	{
		EXPECT_EQ(static_cast<uint32>(page.padding0.get(i)), 0U);
	}
	EXPECT_EQ(static_cast<uint32>(page.padding1.get(0)), 0U);
}

TEST_F(ContractTestingPulseEditorV3, PlayerSummaryCountsAllInterleavedTicketsOnAnOffsetPage)
{
	const auto created = createGame(CREATOR, makeGame(4, 0));
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
	setCalendar(2025, 1, 3);
	fund(PLAYER, 200);
	fund(SECOND_PLAYER, 200);
	ASSERT_EQ(buy(PLAYER, created.gameId, 0).returnCode, PLDT::EReturnCode::SUCCESS);
	ASSERT_EQ(buy(SECOND_PLAYER, created.gameId, 1).returnCode, PLDT::EReturnCode::SUCCESS);
	ASSERT_EQ(buy(PLAYER, created.gameId, 2).returnCode, PLDT::EReturnCode::SUCCESS);
	ASSERT_EQ(buy(SECOND_PLAYER, created.gameId, 3).returnCode, PLDT::EReturnCode::SUCCESS);

	const auto page = players(created.gameId, 1, 1, 1);

	ASSERT_EQ(page.returnCode, PLDT::EReturnCode::SUCCESS);
	ASSERT_EQ(page.totalCount, 2ULL);
	ASSERT_EQ(static_cast<uint32>(page.returnedCount), 1U);
	EXPECT_EQ(page.players.get(0).player, SECOND_PLAYER);
	EXPECT_EQ(page.players.get(0).totalPayout, 0ULL);
	EXPECT_EQ(static_cast<uint32>(page.players.get(0).ticketCount), 2U);
	EXPECT_EQ(static_cast<uint32>(page.players.get(0).winningTicketCount), 0U);
	EXPECT_EQ(static_cast<uint32>(page.players.get(0).paidTicketCount), 0U);
	EXPECT_EQ(static_cast<uint32>(page.players.get(0).bonusQualifiedTicketCount), 0U);
}

TEST_F(ContractTestingPulseEditorV3, GetPlayersAggregatesSettlementAndBonusData)
{
	const Asset bonus{CREATOR, assetNameFromString("PEDPLYR")};
	ASSERT_EQ(issueAsset(bonus, 1), 1);
	ASSERT_EQ(transferAsset(bonus, CREATOR, PLAYER, 1), 1);
	auto configuration = makeGame(3, 100);
	configuration.tierWeightsBps = {};
	configuration.tierWeightsBps.set(PLDT::payoutMatrixIndex(4, 0), 10000);
	configuration.bonusAssets.set(0, bonus);
	configuration.bonusAssetCount = 1;
	configuration.bonusMultiplierBps = 20000;
	configuration.bonusOwnershipManagingContractIndex = QX_CONTRACT_INDEX;
	configuration.bonusPossessionManagingContractIndex = QX_CONTRACT_INDEX;
	const auto created = createGame(CREATOR, configuration);
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
	const m256i digest(61, 62, 63, 64);
	etalonTick.prevSpectrumDigest = digest;
	const auto winning =
	    expectedWinningDigits(digest, created.gameId, 3, configuration.codeLength, configuration.maxDigit, configuration.allowRepeatedDigits);
	const auto losing = nonMatchingDigits(winning, configuration.codeLength, configuration.maxDigit);
	setCalendar(2025, 1, 3);
	fund(PLAYER, 200);
	fund(SECOND_PLAYER, 100);
	ASSERT_EQ(buyDigits(PLAYER, created.gameId, winning).returnCode, PLDT::EReturnCode::SUCCESS);
	ASSERT_EQ(buyDigits(SECOND_PLAYER, created.gameId, winning).returnCode, PLDT::EReturnCode::SUCCESS);
	ASSERT_EQ(buyDigits(PLAYER, created.gameId, losing).returnCode, PLDT::EReturnCode::SUCCESS);
	beginTickAt(100);

	const auto page = players(created.gameId);

	ASSERT_EQ(page.returnCode, PLDT::EReturnCode::SUCCESS);
	ASSERT_EQ(page.totalCount, 2ULL);
	ASSERT_EQ(static_cast<uint32>(page.returnedCount), 2U);
	EXPECT_EQ(page.players.get(0).player, PLAYER);
	EXPECT_EQ(page.players.get(0).totalPayout, 261ULL);
	EXPECT_EQ(static_cast<uint32>(page.players.get(0).ticketCount), 2U);
	EXPECT_EQ(static_cast<uint32>(page.players.get(0).winningTicketCount), 1U);
	EXPECT_EQ(static_cast<uint32>(page.players.get(0).paidTicketCount), 1U);
	EXPECT_EQ(static_cast<uint32>(page.players.get(0).bonusQualifiedTicketCount), 2U);
	EXPECT_EQ(page.players.get(1).player, SECOND_PLAYER);
	EXPECT_EQ(page.players.get(1).totalPayout, 130ULL);
	EXPECT_EQ(static_cast<uint32>(page.players.get(1).ticketCount), 1U);
	EXPECT_EQ(static_cast<uint32>(page.players.get(1).winningTicketCount), 1U);
	EXPECT_EQ(static_cast<uint32>(page.players.get(1).paidTicketCount), 1U);
	EXPECT_EQ(static_cast<uint32>(page.players.get(1).bonusQualifiedTicketCount), 0U);
}

TEST_F(ContractTestingPulseEditorV3, GetPlayersPagesUniquePlayersAndClampsLimit)
{
	constexpr uint16 playerCount = 130;
	const auto created = createGame(CREATOR, makeGame(playerCount, 0));
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
	setCalendar(2025, 1, 3);
	for (uint16 i = 0; i < playerCount; ++i)
	{
		const id player{static_cast<uint64>(1000 + i), static_cast<uint64>(2000 + i), static_cast<uint64>(3000 + i), static_cast<uint64>(4000 + i)};
		fund(player, 100);
		ASSERT_EQ(buy(player, created.gameId, static_cast<uint8>(i % 8)).returnCode, PLDT::EReturnCode::SUCCESS);
	}

	const auto firstPage = players(created.gameId, 1, 0, 0xffff);
	const auto secondPage = players(created.gameId, 1, 64, PLDT_PLAYERS_PAGE_CAPACITY);
	const auto lastPage = players(created.gameId, 1, 128, PLDT_PLAYERS_PAGE_CAPACITY);
	const auto countOnly = players(created.gameId, 1, 0, 0);
	const auto beyondEnd = players(created.gameId, 1, 10000, PLDT_PLAYERS_PAGE_CAPACITY);

	ASSERT_EQ(firstPage.returnCode, PLDT::EReturnCode::SUCCESS);
	ASSERT_EQ(firstPage.totalCount, 130ULL);
	ASSERT_EQ(static_cast<uint32>(firstPage.returnedCount), 64U);
	ASSERT_EQ(secondPage.totalCount, 130ULL);
	ASSERT_EQ(static_cast<uint32>(secondPage.returnedCount), 64U);
	ASSERT_EQ(lastPage.totalCount, 130ULL);
	ASSERT_EQ(static_cast<uint32>(lastPage.returnedCount), 2U);
	for (uint16 i = 0; i < 64; ++i)
	{
		EXPECT_EQ(firstPage.players.get(i).player.u64._0, static_cast<uint64>(1000 + i));
		EXPECT_EQ(secondPage.players.get(i).player.u64._0, static_cast<uint64>(1064 + i));
	}
	EXPECT_EQ(lastPage.players.get(0).player.u64._0, 1128ULL);
	EXPECT_EQ(lastPage.players.get(1).player.u64._0, 1129ULL);
	EXPECT_EQ(lastPage.players.get(2).player, NULL_ID);
	EXPECT_EQ(countOnly.returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(countOnly.totalCount, 130ULL);
	EXPECT_EQ(static_cast<uint32>(countOnly.returnedCount), 0U);
	EXPECT_EQ(countOnly.players.get(0).player, NULL_ID);
	EXPECT_EQ(beyondEnd.returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(beyondEnd.totalCount, 130ULL);
	EXPECT_EQ(static_cast<uint32>(beyondEnd.returnedCount), 0U);
}

TEST_F(ContractTestingPulseEditorV3, GetPlayersKeepsGamesAndPermanentRoundsSeparate)
{
	auto permanentConfiguration = makeGame(1, 0);
	permanentConfiguration.mode = PLDT::EGameMode::PERMANENT;
	const auto permanent = createGame(CREATOR, permanentConfiguration);
	const auto oneShot = createGame(CREATOR, makeGame(1, 0));
	ASSERT_EQ(permanent.returnCode, PLDT::EReturnCode::SUCCESS);
	ASSERT_EQ(oneShot.returnCode, PLDT::EReturnCode::SUCCESS);
	setCalendar(2025, 1, 3);
	fund(PLAYER, 100);
	fund(OUTSIDER, 100);
	ASSERT_EQ(buy(PLAYER, permanent.gameId, 0).returnCode, PLDT::EReturnCode::SUCCESS);
	ASSERT_EQ(buy(OUTSIDER, oneShot.gameId, 1).returnCode, PLDT::EReturnCode::SUCCESS);
	setCalendar(2025, 1, 4);
	beginTickAt(100);
	ASSERT_EQ(game(permanent.gameId).game.roundNumber, 2ULL);
	fund(SECOND_PLAYER, 100);
	ASSERT_EQ(buy(SECOND_PLAYER, permanent.gameId, 2).returnCode, PLDT::EReturnCode::SUCCESS);

	const auto firstRound = players(permanent.gameId, 1);
	const auto secondRound = players(permanent.gameId, 2);
	const auto otherGame = players(oneShot.gameId, 1);

	ASSERT_EQ(static_cast<uint32>(firstRound.returnedCount), 1U);
	EXPECT_EQ(firstRound.players.get(0).player, PLAYER);
	ASSERT_EQ(static_cast<uint32>(secondRound.returnedCount), 1U);
	EXPECT_EQ(secondRound.players.get(0).player, SECOND_PLAYER);
	ASSERT_EQ(static_cast<uint32>(otherGame.returnedCount), 1U);
	EXPECT_EQ(otherGame.players.get(0).player, OUTSIDER);
}

TEST_F(ContractTestingPulseEditorV3, GetPlayersReturnsEmptyRoundAndLookupErrors)
{
	const auto created = createGame(CREATOR, makeGame(1, 0));
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
	const auto empty = players(created.gameId);

	EXPECT_EQ(empty.returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(empty.totalCount, 0ULL);
	EXPECT_EQ(static_cast<uint32>(empty.returnedCount), 0U);
	EXPECT_EQ(empty.players.get(0).player, NULL_ID);
	EXPECT_EQ(players(created.gameId, 2).returnCode, PLDT::EReturnCode::INVALID_ROUND);
	EXPECT_EQ(players(999999, 1).returnCode, PLDT::EReturnCode::INVALID_GAME);
}

TEST_F(ContractTestingPulseEditorV3, RoundResultDistinguishesKnownGamesAndMissingRounds)
{
	const auto created = createGame(CREATOR, makeGame(1, 0));
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(result(created.gameId).returnCode, PLDT::EReturnCode::INVALID_ROUND);
	const auto checkMissing = [this](const uint64 gameId, const uint64 roundNumber, const PLDT::EReturnCode expected)
	{
		PLDT::GetRoundResult_input resultInput{};
		resultInput.roundKey = {gameId, roundNumber};
		EXPECT_EQ((function<PLDT::GetRoundResult_input, PLDT::GetRoundResult_output>(PLDT_FUNCTION_GET_ROUND_RESULT, resultInput)).returnCode,
		          expected);
		EXPECT_EQ(players(gameId, roundNumber).returnCode, expected);
		PLDT::GetPlayerTickets_input ticketsInput{};
		ticketsInput.roundKey = resultInput.roundKey;
		EXPECT_EQ((function<PLDT::GetPlayerTickets_input, PLDT::GetPlayerTickets_output>(PLDT_FUNCTION_GET_PLAYER_TICKETS, ticketsInput)).returnCode,
		          expected);
		PLDT::GetWinners_input winnersInput{};
		winnersInput.roundKey = resultInput.roundKey;
		EXPECT_EQ((function<PLDT::GetWinners_input, PLDT::GetWinners_output>(PLDT_FUNCTION_GET_WINNERS, winnersInput)).returnCode, expected);
	};
	checkMissing(created.gameId, 2, PLDT::EReturnCode::INVALID_ROUND);
	checkMissing(created.gameId, 0, PLDT::EReturnCode::INVALID_ROUND);
	checkMissing(0, 1, PLDT::EReturnCode::INVALID_ROUND);
	checkMissing(999999, 1, PLDT::EReturnCode::INVALID_GAME);
	ASSERT_EQ(stop(CREATOR, created.gameId).returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(result(created.gameId).returnCode, PLDT::EReturnCode::SUCCESS);
	checkMissing(created.gameId, 2, PLDT::EReturnCode::INVALID_ROUND);
}

TEST_F(ContractTestingPulseEditorV3, RoundResultKeepsActiveGameIdentityAfterHistoryEviction)
{
	auto configuration = makeGame(1, 0);
	configuration.mode = PLDT::EGameMode::PERMANENT;
	const auto permanent = createGame(CREATOR, configuration);
	ASSERT_EQ(permanent.returnCode, PLDT::EReturnCode::SUCCESS);
	setCalendar(2025, 1, 4);
	processFirstGameAt(100);
	ASSERT_EQ(result(permanent.gameId).returnCode, PLDT::EReturnCode::SUCCESS);
	configuration = makeGame(1, 0);
	configuration.startAt = DateAndTime(2025, 1, 5, 0, 0, 0);
	configuration.drawAt = DateAndTime(2025, 1, 6, 0, 0, 0);
	for (uint16 i = 0; i < PLDT_RESULT_HISTORY_SIZE; ++i)
	{
		const auto created = createGame(OUTSIDER, configuration);
		ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
		ASSERT_EQ(stop(OUTSIDER, created.gameId).returnCode, PLDT::EReturnCode::SUCCESS);
	}
	ASSERT_EQ(game(permanent.gameId).returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(result(permanent.gameId).returnCode, PLDT::EReturnCode::INVALID_ROUND);
	EXPECT_EQ(players(permanent.gameId, 1).returnCode, PLDT::EReturnCode::INVALID_ROUND);
	EXPECT_EQ(playerTickets(PLAYER, permanent.gameId).returnCode, PLDT::EReturnCode::INVALID_ROUND);
	EXPECT_EQ(winners(permanent.gameId).returnCode, PLDT::EReturnCode::INVALID_ROUND);
}

TEST_F(ContractTestingPulseEditorV3, ResultPagingTraversesOnlyTheCompletedGamesTicketList)
{
	fund(CREATOR, 100);
	const auto configuration = makeGame(2, 100);
	const auto created = createGame(CREATOR, configuration);
	const m256i digest(17, 18, 19, 20);
	etalonTick.prevSpectrumDigest = digest;
	const auto winning =
	    expectedWinningDigits(digest, created.gameId, 2, configuration.codeLength, configuration.maxDigit, configuration.allowRepeatedDigits);
	const auto losing = nonMatchingDigits(winning, configuration.codeLength, configuration.maxDigit);
	setCalendar(2025, 1, 3);
	fund(PLAYER, 200);
	PLDT::BuyTickets_input purchaseInput{};
	purchaseInput.gameId = created.gameId;
	purchaseInput.ticketCount = 2;
	purchaseInput.tickets.set(0, {winning});
	purchaseInput.tickets.set(1, {losing});
	const auto purchase = procedure<PLDT::BuyTickets_input, PLDT::BuyTickets_output>(PLDT_PROCEDURE_BUY_TICKETS, purchaseInput, PLAYER, 200);
	beginTickAt(100);

	const auto owned = playerTickets(PLAYER, created.gameId);
	ASSERT_EQ(owned.returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(owned.totalCount, 2);
	EXPECT_EQ(static_cast<uint32>(owned.returnedCount), 2U);
	EXPECT_EQ(owned.ticketIndexes.get(0), purchase.ticketIndexes.get(0));
	EXPECT_EQ(owned.ticketIndexes.get(1), purchase.ticketIndexes.get(1));
	const auto ownedPage = playerTickets(PLAYER, created.gameId, 1);
	EXPECT_EQ(ownedPage.totalCount, 2);
	EXPECT_EQ(static_cast<uint32>(ownedPage.returnedCount), 1U);
	EXPECT_EQ(ownedPage.ticketIndexes.get(0), purchase.ticketIndexes.get(1));

	const auto paid = winners(created.gameId);
	ASSERT_EQ(paid.returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(paid.totalCount, 2);
	EXPECT_EQ(static_cast<uint32>(paid.returnedCount), 2U);
	const auto paidPage = winners(created.gameId, 1);
	EXPECT_EQ(paidPage.totalCount, 2);
	EXPECT_EQ(static_cast<uint32>(paidPage.returnedCount), 1U);
	EXPECT_EQ(paidPage.ticketIndexes.get(0), purchase.ticketIndexes.get(1));
	EXPECT_EQ(playerTickets(PLAYER, 999999).returnCode, PLDT::EReturnCode::INVALID_GAME);
}

TEST_F(ContractTestingPulseEditorV3, SettlementBudgetPersistsProgressAcrossAutomationCalls)
{
	fund(CREATOR, 100);
	auto configuration = makeGame(65, 100);
	configuration.tierWeightsBps = {};
	configuration.tierWeightsBps.set(PLDT::payoutMatrixIndex(4, 0), 10000);
	const auto created = createGame(CREATOR, configuration);
	// A future game after the busy slot must not reserve any of the current settlement budget.
	auto futureConfiguration = makeGame(2, 0);
	futureConfiguration.startAt = DateAndTime(2025, 1, 4, 0, 0, 0);
	futureConfiguration.drawAt = DateAndTime(2025, 1, 5, 0, 0, 0);
	ASSERT_EQ(createGame(OUTSIDER, futureConfiguration).returnCode, PLDT::EReturnCode::SUCCESS);
	const m256i digest(21, 22, 23, 24);
	etalonTick.prevSpectrumDigest = digest;
	const auto winning =
	    expectedWinningDigits(digest, created.gameId, 65, configuration.codeLength, configuration.maxDigit, configuration.allowRepeatedDigits);
	setCalendar(2025, 1, 3);
	fund(PLAYER, 6500);
	uint16 remaining = 65;
	while (remaining > 0)
	{
		PLDT::BuyTickets_input input{};
		input.gameId = created.gameId;
		input.ticketCount = remaining > PLDT_MAX_BATCH_TICKETS ? PLDT_MAX_BATCH_TICKETS : remaining;
		for (uint16 i = 0; i < input.ticketCount; ++i)
		{
			input.tickets.set(i, {winning});
		}
		const auto purchase =
		    procedure<PLDT::BuyTickets_input, PLDT::BuyTickets_output>(PLDT_PROCEDURE_BUY_TICKETS, input, PLAYER, input.ticketCount * 100);
		ASSERT_EQ(static_cast<uint32>(purchase.acceptedCount), static_cast<uint32>(input.ticketCount));
		remaining -= input.ticketCount;
	}

	processFirstGameAt(100);
	EXPECT_EQ(result(created.gameId).returnCode, PLDT::EReturnCode::INVALID_ROUND);
	processFirstGameAt(200);
	EXPECT_EQ(result(created.gameId).returnCode, PLDT::EReturnCode::INVALID_ROUND);
	processFirstGameAt(300);
	const auto finished = result(created.gameId);
	ASSERT_EQ(finished.returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(static_cast<uint32>(finished.gameResult.ticketCount), 65U);
	EXPECT_EQ(finished.gameResult.totalPaid, 6405);
}

TEST_F(ContractTestingPulseEditorV3, AutomationSharesOneGlobalTicketActionBudgetAcrossGames)
{
	auto configuration = makeGame(65, 0);
	configuration.tierWeightsBps = {};
	configuration.tierWeightsBps.set(PLDT::payoutMatrixIndex(4, 0), 10000);
	configuration.maxDigit = PLDT_MIN_MAX_DIGIT;
	const auto firstGame = createGame(CREATOR, configuration);
	const auto secondGame = createGame(CREATOR, configuration);
	ASSERT_EQ(firstGame.returnCode, PLDT::EReturnCode::SUCCESS);
	ASSERT_EQ(secondGame.returnCode, PLDT::EReturnCode::SUCCESS);
	const m256i digest(25, 26, 27, 28);
	etalonTick.prevSpectrumDigest = digest;
	const auto firstWinning =
	    expectedWinningDigits(digest, firstGame.gameId, 65, configuration.codeLength, configuration.maxDigit, configuration.allowRepeatedDigits);
	const auto secondWinning =
	    expectedWinningDigits(digest, secondGame.gameId, 65, configuration.codeLength, configuration.maxDigit, configuration.allowRepeatedDigits);
	setCalendar(2025, 1, 3);
	fund(PLAYER, 13000);
	for (uint16 gameIndex = 0; gameIndex < 2; ++gameIndex)
	{
		uint16 remaining = 65;
		while (remaining > 0)
		{
			PLDT::BuyTickets_input input{};
			input.gameId = gameIndex == 0 ? firstGame.gameId : secondGame.gameId;
			input.ticketCount = remaining > PLDT_MAX_BATCH_TICKETS ? PLDT_MAX_BATCH_TICKETS : remaining;
			for (uint16 i = 0; i < input.ticketCount; ++i)
			{
				input.tickets.set(i, {gameIndex == 0 ? firstWinning : secondWinning});
			}
			const auto bought =
			    procedure<PLDT::BuyTickets_input, PLDT::BuyTickets_output>(PLDT_PROCEDURE_BUY_TICKETS, input, PLAYER, input.ticketCount * 100);
			ASSERT_EQ(static_cast<uint32>(bought.acceptedCount), static_cast<uint32>(input.ticketCount));
			remaining -= input.ticketCount;
		}
	}

	processFirstGameAt(100);
	const auto* contractState = reinterpret_cast<const PLDT::StateData*>(contractStates[PLDT_CONTRACT_INDEX]);
	uint16 processed = 0;
	for (uint16 i = 0; i < 130; ++i)
	{
		if (contractState->tickets.get(i).status != PLDT::ETicketStatus::ACTIVE || contractState->tickets.get(i).winnerWeight != 0)
		{
			++processed;
		}
	}
	EXPECT_EQ(static_cast<uint32>(processed), 64U);
	EXPECT_EQ(contractState->games.get(secondGame.slot).status, PLDT::EGameStatus::COUNTING);
}

TEST_F(ContractTestingPulseEditorV3, PlatformGovernanceAndQubicRevenueWithdrawalAreOwnerOnly)
{
	const auto initialOwner = platformAccounting().platformOwner;
	ASSERT_NE(initialOwner, NULL_ID);
	EXPECT_EQ(initialOwner, PULSE_TEAM_OWNER);
	EXPECT_EQ(setPlatformConfig(OUTSIDER, OUTSIDER).returnCode, PLDT::EReturnCode::ACCESS_DENIED);

	PLDT::SetPlatformConfig_input invalidConfiguration{};
	invalidConfiguration.platformOwner = NULL_ID;
	invalidConfiguration.developer1 = DEVELOPER1;
	invalidConfiguration.developer2 = DEVELOPER2;
	const auto invalidOwner = procedure<PLDT::SetPlatformConfig_input, PLDT::SetPlatformConfig_output>(PLDT_PROCEDURE_SET_PLATFORM_CONFIG,
	                                                                                                   invalidConfiguration, initialOwner);
	EXPECT_EQ(invalidOwner.returnCode, PLDT::EReturnCode::INVALID_VALUE);
	invalidConfiguration.platformOwner = CREATOR;
	invalidConfiguration.developer1 = NULL_ID;
	const auto invalidRecipient = procedure<PLDT::SetPlatformConfig_input, PLDT::SetPlatformConfig_output>(PLDT_PROCEDURE_SET_PLATFORM_CONFIG,
	                                                                                                       invalidConfiguration, initialOwner);
	EXPECT_EQ(invalidRecipient.returnCode, PLDT::EReturnCode::INVALID_VALUE);

	ASSERT_EQ(setPlatformConfig(initialOwner, CREATOR).returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(setPlatformConfig(OUTSIDER, OUTSIDER).returnCode, PLDT::EReturnCode::ACCESS_DENIED);

	auto configuration = makeGame(1, 0);
	configuration.ticketPrice = 10000;
	const auto created = createGame(CREATOR, configuration);
	setCalendar(2025, 1, 3);
	fund(PLAYER, 10000);
	ASSERT_EQ(buyAtPrice(PLAYER, created.gameId, 0, 10000).returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(withdrawPlatformRevenue(OUTSIDER).returnCode, PLDT::EReturnCode::ACCESS_DENIED);

	const auto withdrawn = withdrawPlatformRevenue(CREATOR);
	EXPECT_EQ(withdrawn.returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(withdrawn.developer1Paid, 2575);
	EXPECT_EQ(withdrawn.developer2Paid, 2575);
	EXPECT_EQ(getBalance(DEVELOPER1), 2575);
	EXPECT_EQ(getBalance(DEVELOPER2), 2575);
}

TEST_F(ContractTestingPulseEditorV3, InitialUnsetDeveloperRecipientsCannotBurnAccruedRevenue)
{
	const auto initialOwner = platformAccounting().platformOwner;
	auto configuration = makeGame(1, 0);
	configuration.ticketPrice = 10000;
	const auto created = createGame(CREATOR, configuration);
	setCalendar(2025, 1, 3);
	fund(PLAYER, 10000);
	ASSERT_EQ(buyAtPrice(PLAYER, created.gameId, 0, 10000).returnCode, PLDT::EReturnCode::SUCCESS);

	const auto blocked = withdrawPlatformRevenue(initialOwner);
	EXPECT_EQ(blocked.returnCode, PLDT::EReturnCode::TRANSFER_FAILED);
	EXPECT_EQ(blocked.developer1Paid, 0);
	EXPECT_EQ(blocked.developer2Paid, 0);
	EXPECT_EQ(platformAccounting().developer1Accrued, 2575);
	EXPECT_EQ(platformAccounting().developer2Accrued, 2575);

	ASSERT_EQ(setPlatformConfig(initialOwner, CREATOR).returnCode, PLDT::EReturnCode::SUCCESS);
	const auto paid = withdrawPlatformRevenue(CREATOR);
	EXPECT_EQ(paid.returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(paid.developer1Paid, 2575);
	EXPECT_EQ(paid.developer2Paid, 2575);
}

TEST_F(ContractTestingPulseEditorV3, AssetPlatformRevenueWithdrawalUsesPersistentCurrencyBucket)
{
	issuePulseEditorSharesTo(SHAREHOLDER);
	ASSERT_EQ(configurePlatform(CREATOR).returnCode, PLDT::EReturnCode::SUCCESS);
	ASSERT_EQ(createWallet(DEVELOPER1).returnCode, PLDT::EReturnCode::SUCCESS);
	ASSERT_EQ(createWallet(DEVELOPER2).returnCode, PLDT::EReturnCode::SUCCESS);
	ASSERT_EQ(createWallet(SHAREHOLDER).returnCode, PLDT::EReturnCode::SUCCESS);
	const Asset currency{CREATOR, assetNameFromString("PEDREV")};
	ASSERT_EQ(issueAsset(currency, 20000), 20000);
	ASSERT_EQ(transferAsset(currency, CREATOR, PLAYER, 10000), 10000);
	ASSERT_EQ(transferAssetManagement(currency, PLAYER, 10000, PLDT_CONTRACT_INDEX), 10000);

	auto configuration = makeGame(1, 0);
	configuration.ticketPrice = 10000;
	configuration.currencyMode = PLDT::ECurrencyMode::ASSET;
	configuration.currencyAsset = currency;
	configuration.ownershipManagingContractIndex = PLDT_CONTRACT_INDEX;
	configuration.possessionManagingContractIndex = PLDT_CONTRACT_INDEX;
	const auto created = createGame(CREATOR, configuration);
	setCalendar(2025, 1, 3);
	ASSERT_EQ(buyAsset(PLAYER, created.gameId, 0).returnCode, PLDT::EReturnCode::SUCCESS);

	EXPECT_EQ(withdrawAssetPlatformRevenue(OUTSIDER, currency).returnCode, PLDT::EReturnCode::ACCESS_DENIED);
	const auto withdrawn = withdrawAssetPlatformRevenue(CREATOR, currency);
	EXPECT_EQ(withdrawn.returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(withdrawn.developer1Paid, 75);
	EXPECT_EQ(withdrawn.developer2Paid, 75);
	EXPECT_EQ(withdrawn.dividendPaid, 150);
	EXPECT_EQ(numberOfPossessedShares(currency.assetName, currency.issuer, DEVELOPER1, DEVELOPER1, PLDT_CONTRACT_INDEX, PLDT_CONTRACT_INDEX), 75);
	EXPECT_EQ(numberOfPossessedShares(currency.assetName, currency.issuer, DEVELOPER2, DEVELOPER2, PLDT_CONTRACT_INDEX, PLDT_CONTRACT_INDEX), 75);
	EXPECT_EQ(numberOfPossessedShares(currency.assetName, currency.issuer, SHAREHOLDER, SHAREHOLDER, PLDT_CONTRACT_INDEX, PLDT_CONTRACT_INDEX), 150);
	EXPECT_EQ(wallet(DEVELOPER1).assets.get(0).balance, 75);
	EXPECT_EQ(wallet(DEVELOPER2).assets.get(0).balance, 75);
	EXPECT_EQ(wallet(SHAREHOLDER).assets.get(0).balance, 150);
}

TEST_F(ContractTestingPulseEditorV3, AssetPlatformRevenuePaysPldtShareholders)
{
	issuePulseEditorSharesTo(SHAREHOLDER);
	ASSERT_EQ(configurePlatform(CREATOR).returnCode, PLDT::EReturnCode::SUCCESS);
	const Asset currency{CREATOR, assetNameFromString("PEDDIV")};
	ASSERT_EQ(issueAsset(currency, 100000), 100000);
	ASSERT_EQ(transferAsset(currency, CREATOR, PLAYER, 100000), 100000);
	ASSERT_EQ(transferAssetManagement(currency, PLAYER, 100000, PLDT_CONTRACT_INDEX), 100000);

	auto configuration = makeGame(1, 0);
	configuration.ticketPrice = 100000;
	configuration.currencyMode = PLDT::ECurrencyMode::ASSET;
	configuration.currencyAsset = currency;
	configuration.ownershipManagingContractIndex = PLDT_CONTRACT_INDEX;
	configuration.possessionManagingContractIndex = PLDT_CONTRACT_INDEX;
	const auto created = createGame(CREATOR, configuration);
	setCalendar(2025, 1, 3);
	ASSERT_EQ(buyAsset(PLAYER, created.gameId, 0).returnCode, PLDT::EReturnCode::SUCCESS);

	const auto withdrawn = withdrawAssetPlatformRevenue(CREATOR, currency);
	EXPECT_EQ(withdrawn.returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(withdrawn.developer1Paid, 750);
	EXPECT_EQ(withdrawn.developer2Paid, 750);
	EXPECT_EQ(withdrawn.dividendPaid, 1500);
	EXPECT_EQ(numberOfPossessedShares(currency.assetName, currency.issuer, SHAREHOLDER, SHAREHOLDER, PLDT_CONTRACT_INDEX, PLDT_CONTRACT_INDEX), 1500);
}

TEST_F(ContractTestingPulseEditorV3, AssetDividendRetryResumesSnapshotWithoutPayingEarlyHolderTwice)
{
	std::vector<std::pair<m256i, unsigned int>> shares{{PLAYER, NUMBER_OF_COMPUTORS / 2}, {SHAREHOLDER, NUMBER_OF_COMPUTORS / 2}};
	issueContractShares(PLDT_CONTRACT_INDEX, shares, false);
	ASSERT_EQ(configurePlatform(CREATOR).returnCode, PLDT::EReturnCode::SUCCESS);
	const Asset currency{CREATOR, assetNameFromString("PEDRTY")};
	ASSERT_EQ(issueAsset(currency, 724), 724);
	ASSERT_EQ(transferAsset(currency, CREATOR, PLAYER, 362), 362);
	ASSERT_EQ(transferAssetManagement(currency, CREATOR, 362, PLDT_CONTRACT_INDEX), 362);
	ASSERT_EQ(transferAssetManagement(currency, PLAYER, 362, PLDT_CONTRACT_INDEX), 362);
	auto configuration = makeGame(2, 0);
	configuration.ticketPrice = 362;
	configuration.currencyMode = PLDT::ECurrencyMode::ASSET;
	configuration.currencyAsset = currency;
	configuration.ownershipManagingContractIndex = PLDT_CONTRACT_INDEX;
	configuration.possessionManagingContractIndex = PLDT_CONTRACT_INDEX;
	const auto created = createGame(CREATOR, configuration);
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
	setCalendar(2025, 1, 3);
	ASSERT_EQ(buyAsset(PLAYER, created.gameId, 0).returnCode, PLDT::EReturnCode::SUCCESS);

	auto* contractState = reinterpret_cast<PLDT::StateData*>(contractStates[PLDT_CONTRACT_INDEX]);
	const auto accountingSlot = static_cast<uint16>(game(created.gameId).game.assetAccountingLink - 1);
	auto accounting = contractState->assetAccounting.get(accountingSlot);
	accounting.developer1Accrued = 0;
	accounting.developer2Accrued = 0;
	accounting.dividendAccrued = 676;
	contractState->assetAccounting.set(accountingSlot, accounting);

	const auto partial = withdrawAssetPlatformRevenue(CREATOR, currency);
	EXPECT_EQ(partial.returnCode, PLDT::EReturnCode::TRANSFER_FAILED);
	EXPECT_EQ(partial.dividendPaid, 338);

	ASSERT_EQ(buyAsset(CREATOR, created.gameId, 1).returnCode, PLDT::EReturnCode::SUCCESS);
	const auto completed = withdrawAssetPlatformRevenue(CREATOR, currency);
	EXPECT_EQ(completed.returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(completed.dividendPaid, 338);
	EXPECT_EQ(numberOfPossessedShares(currency.assetName, currency.issuer, PLAYER, PLAYER, PLDT_CONTRACT_INDEX, PLDT_CONTRACT_INDEX), 338);
	EXPECT_EQ(numberOfPossessedShares(currency.assetName, currency.issuer, SHAREHOLDER, SHAREHOLDER, PLDT_CONTRACT_INDEX, PLDT_CONTRACT_INDEX), 338);
}

TEST_F(ContractTestingPulseEditorV3, SubShareAssetDividendIsPaidAndReleasesTheDrainedBucket)
{
	issuePulseEditorSharesTo(SHAREHOLDER);
	ASSERT_EQ(configurePlatform(CREATOR).returnCode, PLDT::EReturnCode::SUCCESS);
	const Asset currency{CREATOR, assetNameFromString("PEDFREE")};
	ASSERT_EQ(issueAsset(currency, 34), 34);
	ASSERT_EQ(transferAsset(currency, CREATOR, PLAYER, 34), 34);
	ASSERT_EQ(transferAssetManagement(currency, PLAYER, 34, PLDT_CONTRACT_INDEX), 34);
	auto configuration = makeGame(1, 0);
	configuration.ticketPrice = 34;
	configuration.currencyMode = PLDT::ECurrencyMode::ASSET;
	configuration.currencyAsset = currency;
	configuration.ownershipManagingContractIndex = PLDT_CONTRACT_INDEX;
	configuration.possessionManagingContractIndex = PLDT_CONTRACT_INDEX;
	const auto created = createGame(CREATOR, configuration);
	setCalendar(2025, 1, 3);
	ASSERT_EQ(buyAsset(PLAYER, created.gameId, 0).returnCode, PLDT::EReturnCode::SUCCESS);
	beginTickAt(100);
	ASSERT_EQ(result(created.gameId).returnCode, PLDT::EReturnCode::SUCCESS);

	const auto withdrawn = withdrawAssetPlatformRevenue(CREATOR, currency);
	ASSERT_EQ(withdrawn.returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(withdrawn.developer1Paid, 0);
	EXPECT_EQ(withdrawn.developer2Paid, 0);
	EXPECT_EQ(withdrawn.dividendPaid, 1);
	EXPECT_EQ(numberOfPossessedShares(currency.assetName, currency.issuer, SHAREHOLDER, SHAREHOLDER, PLDT_CONTRACT_INDEX, PLDT_CONTRACT_INDEX), 1);
	const auto* contractState = reinterpret_cast<const PLDT::StateData*>(contractStates[PLDT_CONTRACT_INDEX]);
	for (uint16 i = 0; i < contractState->assetAccounting.capacity(); ++i)
	{
		EXPECT_FALSE(contractState->assetAccounting.get(i).isActive);
	}
}

TEST_F(ContractTestingPulseEditorV3, ManagedAssetOwnerCanReleaseSharesBackToQx)
{
	const Asset asset{CREATOR, assetNameFromString("PEDMGT")};
	ASSERT_EQ(issueAsset(asset, 10), 10);
	ASSERT_EQ(transferAssetManagement(asset, CREATOR, 10, PLDT_CONTRACT_INDEX), 10);
	EXPECT_EQ(numberOfPossessedShares(asset.assetName, asset.issuer, CREATOR, CREATOR, PLDT_CONTRACT_INDEX, PLDT_CONTRACT_INDEX), 10);
	fund(CREATOR, 100);
	const auto released = releaseAssetManagement(CREATOR, asset, 10);
	EXPECT_EQ(released.returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(released.transferResult, 100);
	EXPECT_EQ(numberOfPossessedShares(asset.assetName, asset.issuer, CREATOR, CREATOR, QX_CONTRACT_INDEX, QX_CONTRACT_INDEX), 10);
}

TEST_F(ContractTestingPulseEditorV3, DiscoveryAccountingAndDigitValidationExposeCurrentState)
{
	fund(CREATOR, 100);
	const auto created = createGame(CREATOR, makeGame());
	const auto active = games();
	ASSERT_EQ(active.returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(static_cast<uint32>(active.totalActive), 1U);
	EXPECT_EQ(static_cast<uint32>(active.returnedCount), 1U);
	EXPECT_EQ(active.gameIds.get(0), created.gameId);

	const auto accounting = platformAccounting();
	EXPECT_EQ(static_cast<uint32>(accounting.activeGameCount), 1U);
	EXPECT_EQ(accounting.ticketCount, 0);
	EXPECT_EQ(accounting.maxCreatorFeePercent, PLDT_DEFAULT_MAX_CREATOR_FEE_PERCENT);
	EXPECT_EQ(accounting.walletCreationFee, PLDT_DEFAULT_WALLET_CREATION_FEE);
	EXPECT_EQ(static_cast<uint32>(accounting.walletCount), 1U);

	auto repeated = digits(1);
	repeated.set(1, 1);
	EXPECT_EQ(validate(repeated, 2, 1, false).returnCode, PLDT::EReturnCode::INVALID_DIGITS);
	EXPECT_EQ(validate(repeated, 2, 1, true).returnCode, PLDT::EReturnCode::SUCCESS);
	repeated.set(1, 2);
	EXPECT_EQ(validate(repeated, 2, 1, true).returnCode, PLDT::EReturnCode::INVALID_DIGITS);
}

TEST_F(ContractTestingPulseEditorV3, ResultRingEvictsOnlyTheOldestGenerationAwareId)
{
	uint64 firstGameId = 0;
	uint64 secondGameId = 0;
	for (uint16 i = 0; i <= PLDT_RESULT_HISTORY_SIZE; ++i)
	{
		const auto created = createGame(CREATOR, makeGame(1, 0));
		ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
		if (i == 0)
		{
			firstGameId = created.gameId;
		}
		else if (i == 1)
		{
			secondGameId = created.gameId;
		}
		ASSERT_EQ(stop(CREATOR, created.gameId).returnCode, PLDT::EReturnCode::SUCCESS);
	}
	EXPECT_EQ(result(firstGameId).returnCode, PLDT::EReturnCode::INVALID_GAME);
	EXPECT_EQ(playerTickets(CREATOR, firstGameId).returnCode, PLDT::EReturnCode::INVALID_GAME);
	EXPECT_EQ(winners(firstGameId).returnCode, PLDT::EReturnCode::INVALID_GAME);
	EXPECT_EQ(result(secondGameId).returnCode, PLDT::EReturnCode::SUCCESS);
}

TEST_F(ContractTestingPulseEditorV3, PreviewRejectsGrossRoundRevenueAboveMaxAmount)
{
	auto boundaryConfiguration = makeGame(1, 0);
	boundaryConfiguration.ticketPrice = static_cast<uint64>(MAX_AMOUNT);
	EXPECT_EQ(preview(boundaryConfiguration).returnCode, PLDT::EReturnCode::SUCCESS);

	auto configuration = makeGame(1024, 0);
	configuration.ticketPrice = 1059000000000ULL;

	EXPECT_EQ(preview(configuration).returnCode, PLDT::EReturnCode::INVALID_VALUE);
}

TEST_F(ContractTestingPulseEditorV3, WalletDepositDoesNotChangeReservedPrize)
{
	const auto created = createGame(CREATOR, makeGame(2, 100));
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
	depositWalletQubic(CREATOR, 10001);
	EXPECT_EQ(wallet(CREATOR).refundableQubic, 10001ULL);
	EXPECT_EQ(game(created.gameId).game.prizePool, 100ULL);
}

TEST_F(ContractTestingPulseEditorV3, PermanentTicketCannotOverflowFuturePoolReturn)
{
	PLDT::CreateGame_input createInput{};
	createInput = makeGame(2, 100);
	createInput.mode = PLDT::EGameMode::PERMANENT;
	const auto created = createGame(CREATOR, createInput);
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);

	auto* contractState = reinterpret_cast<PLDT::StateData*>(contractStates[PLDT_CONTRACT_INDEX]);
	auto reserved = contractState->games.get(created.slot);
	reserved.prizePool = PLDT_MAX_TRANSFER_AMOUNT - 1;
	contractState->games.set(created.slot, reserved);

	setCalendar(2025, 1, 3);
	fund(PLAYER, 100);
	const sint64 balanceBefore = getBalance(PLAYER);
	const auto purchase = buy(PLAYER, created.gameId, 0);

	EXPECT_EQ(purchase.returnCode, PLDT::EReturnCode::STORAGE_FULL);
	EXPECT_EQ(getBalance(PLAYER), balanceBefore);
	const auto stored = game(created.gameId).game;
	EXPECT_EQ(static_cast<uint32>(stored.ticketCount), 0U);
	EXPECT_EQ(stored.totalRevenue, 0ULL);
	EXPECT_EQ(stored.prizePool, PLDT_MAX_TRANSFER_AMOUNT - 1);
}

TEST_F(ContractTestingPulseEditorV3, PermanentBatchChecksPoolReturnCapacityAtomically)
{
	PLDT::CreateGame_input createInput{};
	createInput = makeGame(2, 100);
	createInput.mode = PLDT::EGameMode::PERMANENT;
	const auto created = createGame(CREATOR, createInput);
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);

	auto* contractState = reinterpret_cast<PLDT::StateData*>(contractStates[PLDT_CONTRACT_INDEX]);
	auto reserved = contractState->games.get(created.slot);
	reserved.prizePool = PLDT_MAX_TRANSFER_AMOUNT - 150;
	contractState->games.set(created.slot, reserved);

	setCalendar(2025, 1, 3);
	fund(PLAYER, 200);
	const sint64 balanceBefore = getBalance(PLAYER);
	const auto purchase = buyBatch(PLAYER, created.gameId, {0, 1});

	EXPECT_EQ(purchase.returnCode, PLDT::EReturnCode::STORAGE_FULL);
	EXPECT_EQ(getBalance(PLAYER), balanceBefore);
	const auto stored = game(created.gameId).game;
	EXPECT_EQ(static_cast<uint32>(stored.ticketCount), 0U);
	EXPECT_EQ(stored.totalRevenue, 0ULL);
	EXPECT_EQ(stored.prizePool, PLDT_MAX_TRANSFER_AMOUNT - 150);
}

TEST_F(ContractTestingPulseEditorV3, QubicTicketBurnRefillsExecutionFeeReserve)
{
	auto configuration = makeGame(1, 0);
	configuration.ticketPrice = 103;
	const auto created = createGame(CREATOR, configuration);
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
	setCalendar(2025, 1, 3);
	setContractFeeReserve(PLDT_CONTRACT_INDEX, 1000);

	ASSERT_EQ(buyAtPrice(PLAYER, created.gameId, 0, 103).returnCode, PLDT::EReturnCode::SUCCESS);

	EXPECT_EQ(getContractFeeReserve(PLDT_CONTRACT_INDEX), 1001);
}

TEST_F(ContractTestingPulseEditorV3, ManagementRightsRequireOwnerOriginator)
{
	INIT_CONTRACT(TESTEXA);
	INIT_CONTRACT(TESTEXB);
	callSystemProcedure(TESTEXA_CONTRACT_INDEX, INITIALIZE);
	callSystemProcedure(TESTEXB_CONTRACT_INDEX, INITIALIZE);
	fund(CREATOR, 1);
	fund(OUTSIDER, 1);

	const Asset asset{CREATOR, assetNameFromString("PEDAUTH")};
	TESTEXA::IssueAsset_input issueInput{asset.assetName, 20, 0, 0};
	TESTEXA::IssueAsset_output issueOutput{};
	ASSERT_TRUE(invokeUserProcedure(TESTEXA_CONTRACT_INDEX, 1, issueInput, issueOutput, CREATOR, 0));
	ASSERT_EQ(issueOutput.issuedNumberOfShares, 20);

	const id testExampleB(TESTEXB_CONTRACT_INDEX, 0, 0, 0);
	fund(testExampleB, 1);
	TESTEXA::TransferShareOwnershipAndPossession_input ownershipInput{};
	ownershipInput.asset = asset;
	ownershipInput.numberOfShares = 10;
	ownershipInput.newOwnerAndPossessor = testExampleB;
	TESTEXA::TransferShareOwnershipAndPossession_output ownershipOutput{};
	ASSERT_TRUE(invokeUserProcedure(TESTEXA_CONTRACT_INDEX, 2, ownershipInput, ownershipOutput, CREATOR, 0));
	ASSERT_EQ(ownershipOutput.transferredNumberOfShares, 10);

	// Exercise PLDT's callback through TestExampleB's valid inter-contract release path.
	const auto originalPreAcquire = contractSystemProcedures[TESTEXB_CONTRACT_INDEX][PRE_ACQUIRE_SHARES];
	const auto originalPreAcquireLocalsSize = contractSystemProcedureLocalsSizes[TESTEXB_CONTRACT_INDEX][PRE_ACQUIRE_SHARES];
	const auto originalPostAcquire = contractSystemProcedures[TESTEXB_CONTRACT_INDEX][POST_ACQUIRE_SHARES];
	const auto originalPostAcquireLocalsSize = contractSystemProcedureLocalsSizes[TESTEXB_CONTRACT_INDEX][POST_ACQUIRE_SHARES];
	auto* const originalTestExampleBState = contractStates[TESTEXB_CONTRACT_INDEX];
	contractSystemProcedures[TESTEXB_CONTRACT_INDEX][PRE_ACQUIRE_SHARES] = contractSystemProcedures[PLDT_CONTRACT_INDEX][PRE_ACQUIRE_SHARES];
	contractSystemProcedureLocalsSizes[TESTEXB_CONTRACT_INDEX][PRE_ACQUIRE_SHARES] =
	    contractSystemProcedureLocalsSizes[PLDT_CONTRACT_INDEX][PRE_ACQUIRE_SHARES];
	contractSystemProcedures[TESTEXB_CONTRACT_INDEX][POST_ACQUIRE_SHARES] = contractSystemProcedures[PLDT_CONTRACT_INDEX][POST_ACQUIRE_SHARES];
	contractSystemProcedureLocalsSizes[TESTEXB_CONTRACT_INDEX][POST_ACQUIRE_SHARES] =
	    contractSystemProcedureLocalsSizes[PLDT_CONTRACT_INDEX][POST_ACQUIRE_SHARES];
	contractStates[TESTEXB_CONTRACT_INDEX] = contractStates[PLDT_CONTRACT_INDEX];
	TESTEXB::GetTestExampleAShareManagementRights_input rightsInput{};
	rightsInput.asset = asset;
	rightsInput.numberOfShares = 10;
	TESTEXB::GetTestExampleAShareManagementRights_output rightsOutput{};
	const bool attackerCallCompleted = invokeUserProcedure(TESTEXB_CONTRACT_INDEX, 7, rightsInput, rightsOutput, OUTSIDER, 0);
	const sint64 attackerTransferredShares = rightsOutput.transferredNumberOfShares;
	const sint64 attackerManagedShares =
	    numberOfPossessedShares(asset.assetName, asset.issuer, testExampleB, testExampleB, TESTEXA_CONTRACT_INDEX, TESTEXA_CONTRACT_INDEX);
	const bool ownerCallCompleted = invokeUserProcedure(TESTEXB_CONTRACT_INDEX, 7, rightsInput, rightsOutput, testExampleB, 0);
	contractStates[TESTEXB_CONTRACT_INDEX] = originalTestExampleBState;
	contractSystemProcedures[TESTEXB_CONTRACT_INDEX][PRE_ACQUIRE_SHARES] = originalPreAcquire;
	contractSystemProcedureLocalsSizes[TESTEXB_CONTRACT_INDEX][PRE_ACQUIRE_SHARES] = originalPreAcquireLocalsSize;
	contractSystemProcedures[TESTEXB_CONTRACT_INDEX][POST_ACQUIRE_SHARES] = originalPostAcquire;
	contractSystemProcedureLocalsSizes[TESTEXB_CONTRACT_INDEX][POST_ACQUIRE_SHARES] = originalPostAcquireLocalsSize;

	ASSERT_TRUE(attackerCallCompleted);
	EXPECT_EQ(attackerTransferredShares, 0);
	EXPECT_EQ(attackerManagedShares, 10);
	ASSERT_TRUE(ownerCallCompleted);
	EXPECT_EQ(rightsOutput.transferredNumberOfShares, 10);
	EXPECT_EQ(numberOfPossessedShares(asset.assetName, asset.issuer, testExampleB, testExampleB, TESTEXB_CONTRACT_INDEX, TESTEXB_CONTRACT_INDEX), 10);
}

TEST_F(ContractTestingPulseEditorV3, UnifiedCreateGameSelectsPermanentModeAndFundsFirstRound)
{
	auto input = makeGame();
	input.mode = PLDT::EGameMode::PERMANENT;
	fund(CREATOR, 30500);

	const auto created = createGame(CREATOR, input);

	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
	const auto stored = game(created.gameId).game;
	EXPECT_EQ(stored.mode, PLDT::EGameMode::PERMANENT);
	EXPECT_EQ(stored.prizePool, 100ULL);
	EXPECT_EQ(wallet(CREATOR).serviceCredit, 989000ULL);
	EXPECT_EQ(wallet(CREATOR).refundableQubic, 0ULL);
}

TEST_F(ContractTestingPulseEditorV3, GovernanceChangesRoundFeeOnlyAfterShareholderApproval)
{
	issuePulseEditorSharesTo(PLAYER);
	const auto proposal = proposeEconomics(PLAYER, 3, 20000);
	ASSERT_NE(static_cast<uint32>(proposal), static_cast<uint32>(INVALID_PROPOSAL_INDEX));
	EXPECT_EQ(platformAccounting().roundFee, 10000ULL);
	ASSERT_TRUE(voteEconomics(PLAYER, proposal));
	callSystemProcedure(PLDT_CONTRACT_INDEX, END_EPOCH);
	EXPECT_EQ(platformAccounting().roundFee, 20000ULL);
}

TEST_F(ContractTestingPulseEditorV3, GovernanceDefaultOperationFeeDebitsOneThousandQu)
{
	ASSERT_EQ(createWallet(CREATOR).returnCode, PLDT::EReturnCode::SUCCESS);
	const auto before = wallet(CREATOR).serviceCredit;
	auto config = makeGame(1, 0);
	config.codeLength = 0;
	const auto result = procedure<PLDT::CreateGame_input, PLDT::CreateGame_output>(PLDT_PROCEDURE_CREATE_GAME, config, CREATOR);
	EXPECT_EQ(result.returnCode, PLDT::EReturnCode::INVALID_VALUE);
	EXPECT_EQ(wallet(CREATOR).serviceCredit, before - 1000);
}

TEST_F(ContractTestingPulseEditorV3, GovernanceTicketRatesStayFixedUntilNextRound)
{
	issuePulseEditorSharesTo(PLAYER);
	// Pin this scenario's economics independently of the deployment's initial burn rate.
	ASSERT_TRUE(voteEconomics(PLAYER, proposeEconomics(PLAYER, 1, 5)));
	callSystemProcedure(PLDT_CONTRACT_INDEX, END_EPOCH);
	++system.epoch;
	auto config = makeGame(4, 0);
	config.mode = PLDT::EGameMode::PERMANENT;
	config.ticketPrice = 10000;
	const auto created = createGame(CREATOR, config);
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
	auto proposal = proposeEconomics(PLAYER, 1, 10);
	ASSERT_NE(static_cast<uint32>(proposal), static_cast<uint32>(INVALID_PROPOSAL_INDEX));
	ASSERT_TRUE(voteEconomics(PLAYER, proposal));
	callSystemProcedure(PLDT_CONTRACT_INDEX, END_EPOCH);
	++system.epoch;
	proposal = proposeEconomics(PLAYER, 0, 10);
	ASSERT_TRUE(voteEconomics(PLAYER, proposal));
	callSystemProcedure(PLDT_CONTRACT_INDEX, END_EPOCH);
	setCalendar(2025, 1, 3);
	fund(SECOND_PLAYER, 20000);
	ASSERT_EQ(buyAtPrice(SECOND_PLAYER, created.gameId, 0, 10000).returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(game(created.gameId).game.prizePool, 9215ULL);
	setCalendar(2025, 1, 4);
	for (uint32 tick = 100; tick <= 400; tick += 100)
	{
		processFirstGameAt(tick);
	}
	ASSERT_EQ(game(created.gameId).game.roundNumber, 2ULL);
	ASSERT_EQ(buyAtPrice(SECOND_PLAYER, created.gameId, 0, 10000).returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(game(created.gameId).game.prizePool, 8100ULL);
}

TEST_F(ContractTestingPulseEditorV3, GovernanceRoundFeeRefreshesAtRollover)
{
	issuePulseEditorSharesTo(PLAYER);
	auto config = makeGame(2, 0);
	config.mode = PLDT::EGameMode::PERMANENT;
	const auto created = createGame(CREATOR, config);
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
	const auto proposal = proposeEconomics(PLAYER, 3, 20000);
	ASSERT_TRUE(voteEconomics(PLAYER, proposal));
	callSystemProcedure(PLDT_CONTRACT_INDEX, END_EPOCH);
	const auto before = wallet(CREATOR).serviceCredit;
	EXPECT_EQ(game(created.gameId).game.roundFeeSnapshot, 10000ULL);
	setCalendar(2025, 1, 4);
	processFirstGameAt(100);
	ASSERT_EQ(game(created.gameId).game.roundNumber, 2ULL);
	EXPECT_EQ(game(created.gameId).game.roundFeeSnapshot, 20000ULL);
	EXPECT_EQ(wallet(CREATOR).serviceCredit, before - 20000);
}

TEST_F(ContractTestingPulseEditorV3, GovernanceIncompatibleCreatorFeeStopsBeforeNextDebit)
{
	issuePulseEditorSharesTo(PLAYER);
	auto config = makeGame(2, 0);
	config.mode = PLDT::EGameMode::PERMANENT;
	config.creatorFeePercent = 20;
	const auto created = createGame(CREATOR, config);
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
	const auto proposal = proposeEconomics(PLAYER, 2, 10);
	ASSERT_TRUE(voteEconomics(PLAYER, proposal));
	callSystemProcedure(PLDT_CONTRACT_INDEX, END_EPOCH);
	const auto before = wallet(CREATOR).serviceCredit;
	setCalendar(2025, 1, 4);
	processFirstGameAt(100);
	EXPECT_EQ(game(created.gameId).returnCode, PLDT::EReturnCode::INVALID_GAME);
	EXPECT_EQ(wallet(CREATOR).serviceCredit, before);
	PLDT::GetRoundResult_input input{};
	input.roundKey.gameId = created.gameId;
	input.roundKey.roundNumber = 1;
	const auto history = function<PLDT::GetRoundResult_input, PLDT::GetRoundResult_output>(2, input);
	EXPECT_EQ(history.gameResult.gameStopReason, PLDT::EGameStopReason::ECONOMICS_INCOMPATIBLE);
}

TEST_F(ContractTestingPulseEditorV3, GovernanceRejectsInvalidValuesAndNonShareholdersAndRefundsRewards)
{
	issuePulseEditorSharesTo(PLAYER);
	// Pin this scenario's economics independently of the deployment's initial burn rate.
	ASSERT_TRUE(voteEconomics(PLAYER, proposeEconomics(PLAYER, 1, 5)));
	callSystemProcedure(PLDT_CONTRACT_INDEX, END_EPOCH);
	++system.epoch;
	EXPECT_EQ(static_cast<uint32>(proposeEconomics(OUTSIDER, 3, 20000)), static_cast<uint32>(INVALID_PROPOSAL_INDEX));
	EXPECT_EQ(static_cast<uint32>(proposeEconomics(PULSE_TEAM_OWNER, 3, 20000)), static_cast<uint32>(INVALID_PROPOSAL_INDEX));
	for (uint64 variable = 0; variable < 6; ++variable)
	{
		EXPECT_EQ(static_cast<uint32>(proposeEconomics(PLAYER, variable, -1)), static_cast<uint32>(INVALID_PROPOSAL_INDEX));
	}
	EXPECT_EQ(static_cast<uint32>(proposeEconomics(PLAYER, 6, 1000)), static_cast<uint32>(INVALID_PROPOSAL_INDEX));
	EXPECT_EQ(static_cast<uint32>(proposeEconomics(PLAYER, 256, 1000)), static_cast<uint32>(INVALID_PROPOSAL_INDEX));
	EXPECT_EQ(static_cast<uint32>(proposeEconomics(PLAYER, 0, 101)), static_cast<uint32>(INVALID_PROPOSAL_INDEX));
	EXPECT_EQ(static_cast<uint32>(proposeEconomics(PLAYER, 1, 81)), static_cast<uint32>(INVALID_PROPOSAL_INDEX));
	EXPECT_EQ(static_cast<uint32>(proposeEconomics(PLAYER, 2, 96)), static_cast<uint32>(INVALID_PROPOSAL_INDEX));
	for (uint64 variable = 3; variable < 6; ++variable)
	{
		EXPECT_EQ(static_cast<uint32>(proposeEconomics(PLAYER, variable, 999)), static_cast<uint32>(INVALID_PROPOSAL_INDEX));
		EXPECT_EQ(static_cast<uint32>(proposeEconomics(PLAYER, variable, MAX_AMOUNT + 1)), static_cast<uint32>(INVALID_PROPOSAL_INDEX));
		EXPECT_NE(static_cast<uint32>(proposeEconomics(PLAYER, variable, 1000)), static_cast<uint32>(INVALID_PROPOSAL_INDEX));
		EXPECT_NE(static_cast<uint32>(proposeEconomics(PLAYER, variable, MAX_AMOUNT)), static_cast<uint32>(INVALID_PROPOSAL_INDEX));
	}
	ProposalDataYesNo proposal{};
	proposal.epoch = system.epoch;
	proposal.type = ProposalTypes::VariableYesNo;
	proposal.data.variableOptions.variable = 3;
	proposal.data.variableOptions.value = 20000;
	fund(PLAYER, 1234);
	const auto before = getBalance(PLAYER);
	EXPECT_NE(static_cast<uint32>((procedure<ProposalDataYesNo, uint16>(65534, proposal, PLAYER, 1234))),
	          static_cast<uint32>(INVALID_PROPOSAL_INDEX));
	EXPECT_EQ(getBalance(PLAYER), before);
	proposal.type = ProposalTypes::TransferYesNo;
	EXPECT_EQ(static_cast<uint32>((procedure<ProposalDataYesNo, uint16>(65534, proposal, PLAYER, 1234))),
	          static_cast<uint32>(INVALID_PROPOSAL_INDEX));
	EXPECT_EQ(getBalance(PLAYER), before);
	proposal.epoch = 0;
	EXPECT_NE(static_cast<uint32>((procedure<ProposalDataYesNo, uint16>(65534, proposal, PLAYER))), static_cast<uint32>(INVALID_PROPOSAL_INDEX));
	EXPECT_FALSE(voteEconomics(PLAYER, 0));
	callSystemProcedure(PLDT_CONTRACT_INDEX, END_EPOCH);
	EXPECT_EQ(platformAccounting().roundFee, 10000ULL);
}

TEST_F(ContractTestingPulseEditorV3, GovernanceUsesNativeQuorumAndRejectsTies)
{
	issuePulseEditorSharesTo(PLAYER);
	const auto proposal = proposeEconomics(PLAYER, 3, 20000);
	ASSERT_NE(static_cast<uint32>(proposal), static_cast<uint32>(INVALID_PROPOSAL_INDEX));
	EXPECT_FALSE(voteEconomics(OUTSIDER, proposal));
	ProposalMultiVoteDataV1 vote{};
	vote.proposalIndex = proposal;
	vote.proposalType = ProposalTypes::VariableYesNo;
	vote.proposalTick = system.tick;
	vote.voteValues.set(0, 1);
	vote.voteCounts.set(0, 450);
	ASSERT_TRUE((procedure<ProposalMultiVoteDataV1, bit>(65535, vote, PLAYER)));
	callSystemProcedure(PLDT_CONTRACT_INDEX, END_EPOCH);
	EXPECT_EQ(platformAccounting().roundFee, 10000ULL);
	vote.voteCounts.set(0, 338);
	vote.voteCounts.set(1, 338);
	ASSERT_TRUE((procedure<ProposalMultiVoteDataV1, bit>(65535, vote, PLAYER)));
	callSystemProcedure(PLDT_CONTRACT_INDEX, END_EPOCH);
	EXPECT_EQ(platformAccounting().roundFee, 10000ULL);
	ASSERT_TRUE(voteEconomics(PLAYER, proposal, 0));
	callSystemProcedure(PLDT_CONTRACT_INDEX, END_EPOCH);
	EXPECT_EQ(platformAccounting().roundFee, 10000ULL);
	vote.voteCounts.set(0, 226);
	vote.voteCounts.set(1, 225);
	ASSERT_TRUE((procedure<ProposalMultiVoteDataV1, bit>(65535, vote, PLAYER)));
	callSystemProcedure(PLDT_CONTRACT_INDEX, END_EPOCH);
	EXPECT_EQ(platformAccounting().roundFee, 20000ULL);
}

TEST_F(ContractTestingPulseEditorV3, GovernanceRevalidatesConflictingPercentagesInProposalOrder)
{
	std::vector<std::pair<m256i, unsigned int>> owners{{PLAYER, 338}, {SECOND_PLAYER, 338}};
	issueContractShares(PLDT_CONTRACT_INDEX, owners, false);
	const auto burn = proposeEconomics(PLAYER, 1, 80);
	const auto creator = proposeEconomics(SECOND_PLAYER, 2, 95);
	ASSERT_LT(static_cast<uint32>(burn), static_cast<uint32>(creator));
	ASSERT_TRUE(voteEconomics(PLAYER, burn));
	ASSERT_TRUE(voteEconomics(SECOND_PLAYER, burn));
	ASSERT_TRUE(voteEconomics(PLAYER, creator));
	ASSERT_TRUE(voteEconomics(SECOND_PLAYER, creator));
	callSystemProcedure(PLDT_CONTRACT_INDEX, END_EPOCH);
	EXPECT_EQ(platformAccounting().burnPercent, 80);
	EXPECT_EQ(platformAccounting().maxCreatorFeePercent, 20);
	++system.epoch;
	const auto first = proposeEconomics(PLAYER, 3, 20000);
	const auto second = proposeEconomics(SECOND_PLAYER, 3, 30000);
	ASSERT_LT(static_cast<uint32>(first), static_cast<uint32>(second));
	for (const auto index : {first, second})
	{
		ASSERT_TRUE(voteEconomics(PLAYER, index));
		ASSERT_TRUE(voteEconomics(SECOND_PLAYER, index));
	}
	callSystemProcedure(PLDT_CONTRACT_INDEX, END_EPOCH);
	EXPECT_EQ(platformAccounting().roundFee, 30000ULL);
}

TEST_F(ContractTestingPulseEditorV3, GovernanceVotingRightsRemainWithProposalSnapshot)
{
	issuePulseEditorSharesTo(PLAYER);
	const auto proposal = proposeEconomics(PLAYER, 3, 20000);
	const Asset shares{NULL_ID, PLDT_CONTRACT_ASSET_NAME};
	ASSERT_EQ(transferAsset(shares, PLAYER, SECOND_PLAYER, 676), 676);
	EXPECT_FALSE(voteEconomics(SECOND_PLAYER, proposal));
	ASSERT_TRUE(voteEconomics(PLAYER, proposal));
	callSystemProcedure(PLDT_CONTRACT_INDEX, END_EPOCH);
	EXPECT_EQ(platformAccounting().roundFee, 20000ULL);
}

TEST_F(ContractTestingPulseEditorV3, GovernanceWalletAndOperationFeesApplyToNewCalls)
{
	issuePulseEditorSharesTo(PLAYER);
	ASSERT_EQ(createWallet(CREATOR).returnCode, PLDT::EReturnCode::SUCCESS);
	const auto proposal = proposeEconomics(PLAYER, 4, 2000);
	ASSERT_TRUE(voteEconomics(PLAYER, proposal));
	callSystemProcedure(PLDT_CONTRACT_INDEX, END_EPOCH);
	EXPECT_EQ(wallet(CREATOR).serviceCredit, 1000000ULL);
	ASSERT_EQ(createWallet(SECOND_PLAYER, 2500).returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(wallet(SECOND_PLAYER).serviceCredit, 2000ULL);
	EXPECT_EQ(wallet(SECOND_PLAYER).refundableQubic, 500ULL);
	++system.epoch;
	ASSERT_TRUE(voteEconomics(PLAYER, proposeEconomics(PLAYER, 5, 2000)));
	callSystemProcedure(PLDT_CONTRACT_INDEX, END_EPOCH);
	auto config = makeGame(1, 0);
	EXPECT_EQ(preview(config).operationFee, 2000ULL);
	config.codeLength = 0;
	const auto before = wallet(SECOND_PLAYER);
	EXPECT_EQ((procedure<PLDT::CreateGame_input, PLDT::CreateGame_output>(1, config, SECOND_PLAYER)).returnCode, PLDT::EReturnCode::INVALID_VALUE);
	EXPECT_EQ(wallet(SECOND_PLAYER).serviceCredit, 0ULL);
	EXPECT_EQ(wallet(SECOND_PLAYER).refundableQubic, before.refundableQubic);
}

TEST_F(ContractTestingPulseEditorV3, GovernanceAssetBurnUsesRoundSnapshotAndAllowsZero)
{
	issuePulseEditorSharesTo(PLAYER);
	// Pin this scenario's economics independently of the deployment's initial burn rate.
	ASSERT_TRUE(voteEconomics(PLAYER, proposeEconomics(PLAYER, 1, 5)));
	callSystemProcedure(PLDT_CONTRACT_INDEX, END_EPOCH);
	++system.epoch;
	const Asset currency{CREATOR, assetNameFromString("GOVBURN")};
	ASSERT_EQ(issueAsset(currency, 20000), 20000);
	ASSERT_EQ(transferAsset(currency, CREATOR, SECOND_PLAYER, 20000), 20000);
	ASSERT_EQ(transferAssetManagement(currency, SECOND_PLAYER, 20000, PLDT_CONTRACT_INDEX), 20000);
	auto config = makeGame(2, 0);
	config.currencyMode = PLDT::ECurrencyMode::ASSET;
	config.currencyAsset = currency;
	config.ownershipManagingContractIndex = PLDT_CONTRACT_INDEX;
	config.possessionManagingContractIndex = PLDT_CONTRACT_INDEX;
	config.ticketPrice = 10000;
	const auto old = createGame(CREATOR, config);
	ASSERT_EQ(old.returnCode, PLDT::EReturnCode::SUCCESS);
	ASSERT_TRUE(voteEconomics(PLAYER, proposeEconomics(PLAYER, 1, 0)));
	callSystemProcedure(PLDT_CONTRACT_INDEX, END_EPOCH);
	const auto newer = createGame(CREATOR, config);
	ASSERT_EQ(newer.returnCode, PLDT::EReturnCode::SUCCESS);
	setCalendar(2025, 1, 3);
	ASSERT_EQ(buyAsset(SECOND_PLAYER, old.gameId, 0).returnCode, PLDT::EReturnCode::SUCCESS);
	ASSERT_EQ(buyAsset(SECOND_PLAYER, newer.gameId, 0).returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(game(old.gameId).game.prizePool, 9215ULL);
	EXPECT_EQ(game(newer.gameId).game.prizePool, 9700ULL);
	EXPECT_EQ(numberOfShares(currency), 19515);
	EXPECT_EQ(numberOfPossessedShares(currency.assetName, currency.issuer, contractId(), contractId(), PLDT_CONTRACT_INDEX, PLDT_CONTRACT_INDEX),
	          19515);
}

TEST_F(ContractTestingPulseEditorV3, GovernanceRetryPreservesHistoryAndChargesNextFeeOnce)
{
	issuePulseEditorSharesTo(PLAYER);
	auto config = makeGame(2, 0);
	config.mode = PLDT::EGameMode::PERMANENT;
	const auto created = createGame(CREATOR, config);
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
	ASSERT_TRUE(voteEconomics(PLAYER, proposeEconomics(PLAYER, 3, 20000)));
	callSystemProcedure(PLDT_CONTRACT_INDEX, END_EPOCH);
	auto* state = reinterpret_cast<PLDT::StateData*>(contractStates[PLDT_CONTRACT_INDEX]);
	state->developer1Accrued = MAX_AMOUNT;
	const auto before = wallet(CREATOR).serviceCredit;
	setCalendar(2025, 1, 4);
	processFirstGameAt(100);
	EXPECT_EQ(game(created.gameId).game.status, PLDT::EGameStatus::FINALIZING);
	EXPECT_EQ(game(created.gameId).game.roundFeeSnapshot, 10000ULL);
	EXPECT_EQ(wallet(CREATOR).serviceCredit, before);
	++system.epoch;
	ASSERT_TRUE(voteEconomics(PLAYER, proposeEconomics(PLAYER, 3, 30000)));
	callSystemProcedure(PLDT_CONTRACT_INDEX, END_EPOCH);
	state->developer1Accrued = 0;
	processFirstGameAt(200);
	EXPECT_EQ(game(created.gameId).game.roundNumber, 2ULL);
	EXPECT_EQ(game(created.gameId).game.roundFeeSnapshot, 30000ULL);
	EXPECT_EQ(result(created.gameId).gameResult.roundFeeSnapshot, 10000ULL);
	EXPECT_EQ(wallet(CREATOR).serviceCredit, before - 30000);
	processFirstGameAt(300);
	EXPECT_EQ(wallet(CREATOR).serviceCredit, before - 30000);
}

TEST_F(ContractTestingPulseEditorV3, GovernanceNativeQueriesExposeProposalVotesAndFees)
{
	issuePulseEditorSharesTo(PLAYER);
	const auto index = proposeEconomics(PLAYER, 3, 20000);
	PLDT::GetShareholderProposal_input request{};
	request.proposalIndex = index;
	const auto proposal = function<PLDT::GetShareholderProposal_input, PLDT::GetShareholderProposal_output>(65533, request);
	EXPECT_EQ(proposal.proposerPubicKey, PLAYER);
	EXPECT_EQ(proposal.proposal.data.variableOptions.variable, 3ULL);
	EXPECT_EQ(proposal.proposal.data.variableOptions.value, 20000);
	PLDT::GetShareholderProposalIndices_input list{};
	list.activeProposals = true;
	list.prevProposalIndex = -1;
	const auto indices = function<PLDT::GetShareholderProposalIndices_input, PLDT::GetShareholderProposalIndices_output>(65532, list);
	EXPECT_EQ(static_cast<uint32>(indices.numOfIndices), 1U);
	EXPECT_EQ(static_cast<uint32>(indices.indices.get(0)), static_cast<uint32>(index));
	const auto fees = function<PLDT::GetShareholderProposalFees_input, PLDT::GetShareholderProposalFees_output>(65531, {});
	EXPECT_EQ(fees.setProposalFee, 0);
	EXPECT_EQ(fees.setVoteFee, 0);
	ASSERT_TRUE(voteEconomics(PLAYER, index));
	PLDT::GetShareholderVotes_input votesRequest{};
	votesRequest.voter = PLAYER;
	votesRequest.proposalIndex = index;
	const auto votes = function<PLDT::GetShareholderVotes_input, PLDT::GetShareholderVotes_output>(65534, votesRequest);
	EXPECT_EQ(votes.voteCounts.get(0), 0U);
	EXPECT_EQ(votes.voteValues.get(1), 1);
	EXPECT_EQ(votes.voteCounts.get(1), 676U);
	PLDT::GetShareholderVotingResults_input resultsRequest{};
	resultsRequest.proposalIndex = index;
	const auto summary = function<PLDT::GetShareholderVotingResults_input, PLDT::GetShareholderVotingResults_output>(65535, resultsRequest);
	EXPECT_EQ(summary.totalVotesCasted, 676U);
	EXPECT_EQ(summary.getAcceptedOption(), 1);
	EXPECT_NE(contractSystemProcedures[PLDT_CONTRACT_INDEX][SET_SHAREHOLDER_PROPOSAL], nullptr);
	EXPECT_NE(contractSystemProcedures[PLDT_CONTRACT_INDEX][SET_SHAREHOLDER_VOTES], nullptr);
}

TEST_F(ContractTestingPulseEditorV3, GovernanceSupportsPercentageEndpointsWithoutUnderflow)
{
	issuePulseEditorSharesTo(PLAYER);
	ASSERT_TRUE(voteEconomics(PLAYER, proposeEconomics(PLAYER, 2, 0)));
	callSystemProcedure(PLDT_CONTRACT_INDEX, END_EPOCH);
	++system.epoch;
	ASSERT_TRUE(voteEconomics(PLAYER, proposeEconomics(PLAYER, 1, 100)));
	callSystemProcedure(PLDT_CONTRACT_INDEX, END_EPOCH);
	auto config = makeGame(1, 0);
	config.ticketPrice = 10000;
	auto amounts = preview(config);
	ASSERT_EQ(amounts.returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(amounts.platformFee, 300ULL);
	EXPECT_EQ(amounts.burn, 9700ULL);
	EXPECT_EQ(amounts.prizeContribution, 0ULL);
	++system.epoch;
	ASSERT_TRUE(voteEconomics(PLAYER, proposeEconomics(PLAYER, 0, 100)));
	callSystemProcedure(PLDT_CONTRACT_INDEX, END_EPOCH);
	amounts = preview(config);
	EXPECT_EQ(amounts.platformFee, 10000ULL);
	EXPECT_EQ(amounts.burn, 0ULL);
	EXPECT_EQ(amounts.prizeContribution, 0ULL);
	++system.epoch;
	ASSERT_TRUE(voteEconomics(PLAYER, proposeEconomics(PLAYER, 0, 0)));
	callSystemProcedure(PLDT_CONTRACT_INDEX, END_EPOCH);
	amounts = preview(config);
	EXPECT_EQ(amounts.platformFee, 0ULL);
	EXPECT_EQ(amounts.burn, 10000ULL);
	EXPECT_EQ(amounts.prizeContribution, 0ULL);
}

TEST_F(ContractTestingPulseEditorV3, GovernanceEnablingBurnStopsNullIssuerAssetAtRollover)
{
	issuePulseEditorSharesTo(PLAYER);
	ASSERT_EQ(transferAssetManagement(Asset{NULL_ID, PLDT_CONTRACT_ASSET_NAME}, PLAYER, 676, PLDT_CONTRACT_INDEX), 676);
	ASSERT_TRUE(voteEconomics(PLAYER, proposeEconomics(PLAYER, 1, 0)));
	callSystemProcedure(PLDT_CONTRACT_INDEX, END_EPOCH);
	auto config = makeGame(2, 0);
	config.mode = PLDT::EGameMode::PERMANENT;
	config.currencyMode = PLDT::ECurrencyMode::ASSET;
	config.currencyAsset = Asset{NULL_ID, PLDT_CONTRACT_ASSET_NAME};
	config.ownershipManagingContractIndex = PLDT_CONTRACT_INDEX;
	config.possessionManagingContractIndex = PLDT_CONTRACT_INDEX;
	const auto created = createGame(CREATOR, config);
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
	++system.epoch;
	ASSERT_TRUE(voteEconomics(PLAYER, proposeEconomics(PLAYER, 1, 5)));
	callSystemProcedure(PLDT_CONTRACT_INDEX, END_EPOCH);
	const auto before = wallet(CREATOR).serviceCredit;
	setCalendar(2025, 1, 4);
	processFirstGameAt(100);
	EXPECT_EQ(game(created.gameId).returnCode, PLDT::EReturnCode::INVALID_GAME);
	EXPECT_EQ(result(created.gameId).gameResult.gameStopReason, PLDT::EGameStopReason::ECONOMICS_INCOMPATIBLE);
	EXPECT_EQ(wallet(CREATOR).serviceCredit, before);
}

TEST_F(ContractTestingPulseEditorV3, GovernanceChecksPendingCreatorFeeBeforeRollover)
{
	issuePulseEditorSharesTo(PLAYER);
	auto config = makeGame(2, 0);
	config.mode = PLDT::EGameMode::PERMANENT;
	const auto created = createGame(CREATOR, config);
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
	PLDT::UpdateGameEconomics_input update{};
	update.gameId = created.gameId;
	update.ticketPrice = 100;
	update.ticketLimit = 2;
	update.playerTicketLimit = 2;
	update.creatorFeePercent = 20;
	ASSERT_EQ((procedure<PLDT::UpdateGameEconomics_input, PLDT::UpdateGameEconomics_output>(5, update, CREATOR)).returnCode,
	          PLDT::EReturnCode::SUCCESS);
	ASSERT_TRUE(voteEconomics(PLAYER, proposeEconomics(PLAYER, 2, 10)));
	callSystemProcedure(PLDT_CONTRACT_INDEX, END_EPOCH);
	const auto before = wallet(CREATOR).serviceCredit;
	setCalendar(2025, 1, 4);
	processFirstGameAt(100);
	EXPECT_EQ(result(created.gameId).gameResult.gameStopReason, PLDT::EGameStopReason::ECONOMICS_INCOMPATIBLE);
	EXPECT_EQ(wallet(CREATOR).serviceCredit, before);
}

TEST_F(ContractTestingPulseEditorV3, GovernanceIntegrationHigherRoundFeeStopsWithoutPartialDebit)
{
	issuePulseEditorSharesTo(SHAREHOLDER);
	auto configuration = makeGame(2, 100);
	configuration.mode = PLDT::EGameMode::PERMANENT;
	const auto created = createGame(CREATOR, configuration);
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
	ASSERT_TRUE(voteEconomics(SHAREHOLDER, proposeEconomics(SHAREHOLDER, 3, 1000000)));
	callSystemProcedure(PLDT_CONTRACT_INDEX, END_EPOCH);
	ASSERT_EQ(platformAccounting().roundFee, 1000000ULL);
	const auto accountingBefore = platformAccounting();
	const auto serviceBefore = wallet(CREATOR).serviceCredit;
	ASSERT_EQ(wallet(CREATOR).refundableQubic, 0ULL);

	setCalendar(2025, 1, 4);
	processFirstGameAt(100);
	EXPECT_EQ(game(created.gameId).returnCode, PLDT::EReturnCode::INVALID_GAME);
	const auto completed = result(created.gameId);
	ASSERT_EQ(completed.returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(completed.gameResult.gameStopReason, PLDT::EGameStopReason::OUT_OF_FUNDS);
	EXPECT_EQ(completed.gameResult.roundFeeSnapshot, 10000ULL);
	EXPECT_EQ(wallet(CREATOR).serviceCredit, serviceBefore);
	EXPECT_EQ(wallet(CREATOR).refundableQubic, 100ULL);
	EXPECT_EQ(static_cast<uint32>(wallet(CREATOR).activeGameCount), 0U);
	EXPECT_TRUE(wallet(CREATOR).serviceCreditUnlocked);
	EXPECT_EQ(platformAccounting().developer1Accrued, accountingBefore.developer1Accrued);
	EXPECT_EQ(platformAccounting().developer2Accrued, accountingBefore.developer2Accrued);
	EXPECT_EQ(platformAccounting().dividendAccrued, accountingBefore.dividendAccrued);
	processFirstGameAt(200);
	EXPECT_EQ(wallet(CREATOR).serviceCredit, serviceBefore);
	EXPECT_EQ(wallet(CREATOR).refundableQubic, 100ULL);
	EXPECT_EQ(platformAccounting().resultCounter, 1ULL);
}

TEST_F(ContractTestingPulseEditorV3, GovernanceIntegrationLowerBurnRejectsExcessiveNextRoundRefund)
{
	issuePulseEditorSharesTo(SHAREHOLDER);
	ASSERT_TRUE(voteEconomics(SHAREHOLDER, proposeEconomics(SHAREHOLDER, 1, 5)));
	callSystemProcedure(PLDT_CONTRACT_INDEX, END_EPOCH);
	++system.epoch;
	const Asset currency{CREATOR, assetNameFromString("GOVCAP")};
	ASSERT_EQ(issueAsset(currency, MAX_AMOUNT), MAX_AMOUNT);
	ASSERT_EQ(transferAssetManagement(currency, CREATOR, MAX_AMOUNT, PLDT_CONTRACT_INDEX), MAX_AMOUNT);
	auto configuration = makeGame(2, MAX_AMOUNT - 190);
	configuration.mode = PLDT::EGameMode::PERMANENT;
	configuration.currencyMode = PLDT::ECurrencyMode::ASSET;
	configuration.currencyAsset = currency;
	configuration.ownershipManagingContractIndex = PLDT_CONTRACT_INDEX;
	configuration.possessionManagingContractIndex = PLDT_CONTRACT_INDEX;
	// At price 100, net is 97 and the configured 5% burn is 4: two tickets can return 186 shares.
	ASSERT_EQ(preview(configuration).returnCode, PLDT::EReturnCode::SUCCESS);
	const auto created = createGame(CREATOR, configuration);
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
	ASSERT_EQ(wallet(CREATOR).assets.get(0).balance, 190ULL);
	ASSERT_TRUE(voteEconomics(SHAREHOLDER, proposeEconomics(SHAREHOLDER, 1, 0)));
	callSystemProcedure(PLDT_CONTRACT_INDEX, END_EPOCH);
	ASSERT_EQ(platformAccounting().burnPercent, 0);
	const auto accountingBefore = platformAccounting();
	const auto serviceBefore = wallet(CREATOR).serviceCredit;

	// Removing burn raises the possible return to seed + 194, which exceeds MAX_AMOUNT by four.
	setCalendar(2025, 1, 4);
	processFirstGameAt(100);
	EXPECT_EQ(game(created.gameId).returnCode, PLDT::EReturnCode::INVALID_GAME);
	const auto completed = result(created.gameId);
	ASSERT_EQ(completed.returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(completed.gameResult.gameStopReason, PLDT::EGameStopReason::ECONOMICS_INCOMPATIBLE);
	EXPECT_EQ(completed.gameResult.burnPercentSnapshot, 5);
	EXPECT_EQ(wallet(CREATOR).serviceCredit, serviceBefore);
	EXPECT_EQ(wallet(CREATOR).assets.get(0).balance, static_cast<uint64>(MAX_AMOUNT));
	EXPECT_EQ(numberOfPossessedShares(currency.assetName, currency.issuer, CREATOR, CREATOR, PLDT_CONTRACT_INDEX, PLDT_CONTRACT_INDEX), MAX_AMOUNT);
	EXPECT_EQ(numberOfPossessedShares(currency.assetName, currency.issuer, contractId(), contractId(), PLDT_CONTRACT_INDEX, PLDT_CONTRACT_INDEX), 0);
	EXPECT_EQ(platformAccounting().developer1Accrued, accountingBefore.developer1Accrued);
	EXPECT_EQ(platformAccounting().developer2Accrued, accountingBefore.developer2Accrued);
	EXPECT_EQ(platformAccounting().dividendAccrued, accountingBefore.dividendAccrued);
	processFirstGameAt(200);
	EXPECT_EQ(wallet(CREATOR).assets.get(0).balance, static_cast<uint64>(MAX_AMOUNT));
	EXPECT_EQ(platformAccounting().resultCounter, 1ULL);
}

TEST_F(ContractTestingPulseEditorV3, GovernanceIntegrationIncompatibleStopWaitsForWinnerAndCreatorTransfers)
{
	issuePulseEditorSharesTo(SHAREHOLDER);
	ASSERT_TRUE(voteEconomics(SHAREHOLDER, proposeEconomics(SHAREHOLDER, 1, 5)));
	callSystemProcedure(PLDT_CONTRACT_INDEX, END_EPOCH);
	++system.epoch;
	const Asset currency{CREATOR, assetNameFromString("GOVPAY")};
	ASSERT_EQ(issueAsset(currency, 400), 400);
	ASSERT_EQ(transferAsset(currency, CREATOR, PLAYER, 100), 100);
	ASSERT_EQ(transferAssetManagement(currency, CREATOR, 300, PLDT_CONTRACT_INDEX), 300);
	ASSERT_EQ(transferAssetManagement(currency, PLAYER, 100, PLDT_CONTRACT_INDEX), 100);
	auto configuration = makeGame(1, 100);
	configuration.mode = PLDT::EGameMode::PERMANENT;
	configuration.currencyMode = PLDT::ECurrencyMode::ASSET;
	configuration.currencyAsset = currency;
	configuration.ownershipManagingContractIndex = PLDT_CONTRACT_INDEX;
	configuration.possessionManagingContractIndex = PLDT_CONTRACT_INDEX;
	configuration.creatorFeePercent = 10;
	const auto created = createGame(CREATOR, configuration);
	ASSERT_EQ(created.returnCode, PLDT::EReturnCode::SUCCESS);
	const m256i digest(13, 14, 15, 16);
	etalonTick.prevSpectrumDigest = digest;
	const auto winning =
	    expectedWinningDigits(digest, created.gameId, 1, configuration.codeLength, configuration.maxDigit, configuration.allowRepeatedDigits);
	PLDT::BuyTickets_input purchase{};
	purchase.gameId = created.gameId;
	purchase.ticketCount = 1;
	purchase.tickets.set(0, {winning});
	setCalendar(2025, 1, 3);
	const auto bought = procedure<PLDT::BuyTickets_input, PLDT::BuyTickets_output>(15, purchase, PLAYER);
	ASSERT_EQ(bought.returnCode, PLDT::EReturnCode::SUCCESS);
	ASSERT_TRUE(voteEconomics(SHAREHOLDER, proposeEconomics(SHAREHOLDER, 2, 0)));
	callSystemProcedure(PLDT_CONTRACT_INDEX, END_EPOCH);
	ASSERT_EQ(platformAccounting().maxCreatorFeePercent, 0);
	const auto serviceBefore = wallet(CREATOR).serviceCredit;
	const auto accountingBefore = platformAccounting();

	// Leave only the three platform shares in custody so the 184-share winner payment must retry.
	{
		QpiContextSystemProcedureCall qpi(PLDT_CONTRACT_INDEX, BEGIN_TICK);
		ASSERT_GE(qpi.transferShareOwnershipAndPossession(currency.assetName, currency.issuer, contractId(), contractId(), 193, OUTSIDER), 0);
	}
	processFirstGameAt(100);
	ASSERT_EQ(game(created.gameId).game.status, PLDT::EGameStatus::PAYING);
	EXPECT_EQ(game(created.gameId).game.totalPaid, 0ULL);
	EXPECT_EQ(ticket(bought.ticketIndexes.get(0)).ticket.status, PLDT::ETicketStatus::ACTIVE);
	EXPECT_EQ(wallet(PLAYER).assets.get(0).balance, 0ULL);
	EXPECT_EQ(wallet(CREATOR).assets.get(0).balance, 200ULL);
	EXPECT_EQ(platformAccounting().resultCounter, 0ULL);

	// Restore the winner's funds, keeping the nine creator shares unavailable for a separate retry.
	{
		QpiContextSystemProcedureCall qpi(PLDT_CONTRACT_INDEX, BEGIN_TICK);
		ASSERT_GE(qpi.transferShareOwnershipAndPossession(currency.assetName, currency.issuer, OUTSIDER, OUTSIDER, 184, contractId()), 0);
	}
	processFirstGameAt(200);
	ASSERT_EQ(game(created.gameId).game.status, PLDT::EGameStatus::FINALIZING);
	EXPECT_EQ(game(created.gameId).game.totalPaid, 184ULL);
	EXPECT_EQ(game(created.gameId).game.pendingCreatorCurrencyPayout, 9ULL);
	EXPECT_EQ(ticket(bought.ticketIndexes.get(0)).ticket.status, PLDT::ETicketStatus::PAID);
	EXPECT_EQ(wallet(PLAYER).assets.get(0).balance, 184ULL);
	EXPECT_EQ(wallet(CREATOR).assets.get(0).balance, 200ULL);
	EXPECT_EQ(wallet(CREATOR).serviceCredit, serviceBefore);
	EXPECT_EQ(platformAccounting().resultCounter, 0ULL);
	processFirstGameAt(300);
	EXPECT_EQ(wallet(PLAYER).assets.get(0).balance, 184ULL);
	EXPECT_EQ(game(created.gameId).game.pendingCreatorCurrencyPayout, 9ULL);
	EXPECT_EQ(platformAccounting().resultCounter, 0ULL);

	{
		QpiContextSystemProcedureCall qpi(PLDT_CONTRACT_INDEX, BEGIN_TICK);
		ASSERT_GE(qpi.transferShareOwnershipAndPossession(currency.assetName, currency.issuer, OUTSIDER, OUTSIDER, 9, contractId()), 0);
	}
	processFirstGameAt(400);
	EXPECT_EQ(game(created.gameId).returnCode, PLDT::EReturnCode::INVALID_GAME);
	const auto completed = result(created.gameId);
	ASSERT_EQ(completed.returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(completed.gameResult.gameStopReason, PLDT::EGameStopReason::ECONOMICS_INCOMPATIBLE);
	EXPECT_EQ(completed.gameResult.terminalReason, PLDT::EGameTerminalReason::SETTLED);
	EXPECT_EQ(completed.gameResult.totalPaid, 184ULL);
	EXPECT_EQ(wallet(CREATOR).assets.get(0).balance, 209ULL);
	EXPECT_EQ(wallet(PLAYER).assets.get(0).balance, 184ULL);
	EXPECT_EQ(wallet(CREATOR).serviceCredit, serviceBefore);
	EXPECT_EQ(static_cast<uint32>(wallet(CREATOR).activeGameCount), 0U);
	EXPECT_EQ(platformAccounting().developer1Accrued, accountingBefore.developer1Accrued);
	EXPECT_EQ(platformAccounting().developer2Accrued, accountingBefore.developer2Accrued);
	EXPECT_EQ(platformAccounting().dividendAccrued, accountingBefore.dividendAccrued);
	processFirstGameAt(500);
	EXPECT_EQ(wallet(CREATOR).assets.get(0).balance, 209ULL);
	EXPECT_EQ(wallet(PLAYER).assets.get(0).balance, 184ULL);
	EXPECT_EQ(platformAccounting().resultCounter, 1ULL);
	EXPECT_EQ(numberOfShares(currency), 396);
}

TEST_F(ContractTestingPulseEditorV3, GovernanceIntegrationContractCallbackEnforcesPulseEditorParameterBounds)
{
	INIT_CONTRACT(TESTEXB);
	const id shareholderContract{TESTEXB_CONTRACT_INDEX, 0, 0, 0};
	issuePulseEditorSharesTo(shareholderContract);
	fund(PLAYER, 1);
	TESTEXB::SetProposalInOtherContractAsShareholder_input input{};
	input.otherContractIndex = PLDT_CONTRACT_INDEX;
	TESTEXB::SetProposalInOtherContractAsShareholder_output output{};
	ProposalDataYesNo proposal{};
	proposal.epoch = system.epoch;
	proposal.type = ProposalTypes::VariableYesNo;
	// These values are valid native proposals but violate PulseEditor's economic constraints.
	for (const auto& invalid : std::vector<std::pair<uint64, sint64>>{{0, 101}, {1, 81}, {2, 100}, {3, 999}, {4, MAX_AMOUNT + 1}, {5, 0}, {6, 1000}})
	{
		proposal.data.variableOptions.variable = invalid.first;
		proposal.data.variableOptions.value = invalid.second;
		copyToBuffer(input.proposalDataBuffer, proposal, true);
		ASSERT_TRUE(invokeUserProcedure(TESTEXB_CONTRACT_INDEX, 40, input, output, PLAYER, 0));
		EXPECT_EQ(static_cast<uint32>(output.proposalIndex), static_cast<uint32>(INVALID_PROPOSAL_INDEX));
	}
	proposal.data.variableOptions.variable = 1;
	proposal.data.variableOptions.value = 10;
	copyToBuffer(input.proposalDataBuffer, proposal, true);
	ASSERT_TRUE(invokeUserProcedure(TESTEXB_CONTRACT_INDEX, 40, input, output, PLAYER, 0));
	ASSERT_NE(static_cast<uint32>(output.proposalIndex), static_cast<uint32>(INVALID_PROPOSAL_INDEX));
	PLDT::GetShareholderProposal_input lookup{};
	lookup.proposalIndex = output.proposalIndex;
	const auto stored = function<PLDT::GetShareholderProposal_input, PLDT::GetShareholderProposal_output>(65533, lookup);
	ASSERT_EQ(stored.proposerPubicKey, shareholderContract);
	ASSERT_EQ(stored.proposal.data.variableOptions.value, 10);
	TESTEXB::SetVotesInOtherContractAsShareholder_input vote{};
	vote.otherContractIndex = PLDT_CONTRACT_INDEX;
	vote.voteData.proposalIndex = output.proposalIndex;
	vote.voteData.proposalType = ProposalTypes::VariableYesNo;
	vote.voteData.proposalTick = stored.proposal.tick;
	vote.voteData.voteValues.set(0, 1);
	TESTEXB::SetVotesInOtherContractAsShareholder_output voted{};
	ASSERT_TRUE(invokeUserProcedure(TESTEXB_CONTRACT_INDEX, 41, vote, voted, PLAYER, 0));
	ASSERT_TRUE(voted.success);
	EXPECT_EQ(platformAccounting().burnPercent, PLDT_DEFAULT_BURN_PERCENT);
	callSystemProcedure(PLDT_CONTRACT_INDEX, END_EPOCH);
	EXPECT_EQ(platformAccounting().burnPercent, 10);
	auto configuration = makeGame(1, 0);
	configuration.ticketPrice = 1000;
	const auto amounts = preview(configuration);
	ASSERT_EQ(amounts.returnCode, PLDT::EReturnCode::SUCCESS);
	EXPECT_EQ(amounts.platformFee, 30ULL);
	EXPECT_EQ(amounts.burn, 97ULL);
	EXPECT_EQ(amounts.prizeContribution, 873ULL);
}
