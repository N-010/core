/**
 * @file PulseEditor.h
 * @brief Paid one-shot and permanent code-guessing games with tiered pool sharing.
 *
 * A permanent game retains immutable rules and advances one non-overlapping
 * current round at a time. There are no templates, fixed payouts, or
 * minimum-player thresholds.
 */

using namespace QPI;

/** Maximum number of game slots retained in contract state. */
constexpr uint16 PLDT_MAX_GAMES = 1024;
/** Maximum concurrently active games owned by one creator. */
constexpr uint16 PLDT_MAX_ACTIVE_GAMES_PER_CREATOR = 16;
/** Longest permitted interval between game creation and draw, in UTC days. */
constexpr sint64 PLDT_MAX_SCHEDULE_DAYS = 366;
/** Capacity of the shared generation-aware ticket store. */
constexpr uint32 PLDT_MAX_TICKETS = 1024 * PLDT_MAX_GAMES;
/** Number of recent round results guaranteed to remain queryable. */
constexpr uint16 PLDT_RESULT_HISTORY_SIZE = 1024;
/** Result slots retained so ticket reclamation can lag behind public history. */
constexpr uint16 PLDT_RESULT_STORAGE_SIZE = PLDT_RESULT_HISTORY_SIZE * 2;
/** Maximum tickets accepted by one round. */
constexpr uint16 PLDT_MAX_TICKETS_PER_GAME = 1024;
/** Maximum tickets accepted atomically by one batch purchase. */
constexpr uint16 PLDT_MAX_BATCH_TICKETS = 16;
/** Maximum asset holdings that may qualify a player for the bonus multiplier. */
constexpr uint16 PLDT_MAX_BONUS_ASSETS = 8;
/** Maximum count of digits in a submitted or winning code. */
constexpr uint8 PLDT_MAX_CODE_LENGTH = 10;
/** Power-of-two digit-array capacity required by QPI Array. */
constexpr uint8 PLDT_DIGITS_ALIGNED = 16;
/** Largest digit value supported by the fixed uniqueness workspace. */
constexpr uint8 PLDT_MAX_DIGIT = PLDT_MAX_CODE_LENGTH - 1;
/** Power-of-two capacity of the digit-presence workspace. */
constexpr uint8 PLDT_DIGIT_BUCKETS = 16;
/** Number of possible exact/misplaced match tiers for the maximum code length. */
constexpr uint16 PLDT_TIER_CAPACITY = div<uint16>(((PLDT_MAX_CODE_LENGTH + 1) * (PLDT_MAX_CODE_LENGTH + 2)), 2);
/** First tier-matrix segment sized within QPI Array limits. */
constexpr uint16 PLDT_TIER_PREFIX_CAPACITY = 64;
/** Second tier-matrix segment containing the remaining match tiers. */
constexpr uint16 PLDT_TIER_SUFFIX_CAPACITY = PLDT_TIER_CAPACITY - PLDT_TIER_PREFIX_CAPACITY;
/** Basis-point denominator used for prize-tier weights. */
constexpr uint32 PLDT_TIER_BPS_SCALE = 10000;
/** Basis-point denominator representing a 1x winner weight. */
constexpr uint32 PLDT_BONUS_MULTIPLIER_SCALE = 10000;
/** Largest supported bonus winner weight, in basis points. */
constexpr uint32 PLDT_MAX_BONUS_MULTIPLIER_BPS = 100000;
/** Ticket-price percentage accrued to platform recipients. */
constexpr uint8 PLDT_PLATFORM_FEE_PERCENT = 3;
/** Ticket-price percentage removed from circulation. */
constexpr uint8 PLDT_BURN_PERCENT = 5;
/** Absolute creator-fee ceiling after reserving the burn share. */
constexpr uint8 PLDT_MAX_CREATOR_FEE_PERCENT = 100 - PLDT_BURN_PERCENT;
/** Developer-one share of the platform fee, in percent. */
constexpr uint8 PLDT_PLATFORM_DEV1_SHARE_PERCENT = 25;
/** Developer-two share of the platform fee, in percent. */
constexpr uint8 PLDT_PLATFORM_DEV2_SHARE_PERCENT = 25;
/** Initial owner-configurable ceiling for creator fees. */
constexpr uint8 PLDT_DEFAULT_MAX_CREATOR_FEE_PERCENT = 20;
/** Default Qubic automation fee reserved per game round. */
constexpr uint64 PLDT_DEFAULT_ROUND_FEE = 10000;
/** Tick interval between automated lifecycle scans. */
constexpr uint32 PLDT_TICK_UPDATE_PERIOD = 100;
/** Number of game slots inspected during one automation pass. */
constexpr uint16 PLDT_AUTOMATION_GAMES_PER_TICK = 32;
/** Maximum settlement or reclamation actions executed per scan. */
constexpr uint16 PLDT_SETTLEMENT_ACTION_BUDGET = 64;
/** Maximum collision retries while generating unique winning digits. */
constexpr uint8 PLDT_RANDOM_RETRY_LIMIT = 32;
/** Little-endian asset name PLDT used for shareholder accounting. */
constexpr uint64 PLDT_CONTRACT_ASSET_NAME = 0x54444c50ULL; // "PLDT"
/** Largest ledger or transfer value accepted by QPI. */
constexpr uint64 PLDT_MAX_TRANSFER_AMOUNT = MAX_AMOUNT;
/** Out-of-band unsigned value used when no slot or result exists. */
constexpr uint64 PLDT_UINT64_SENTINEL = 0xffffffffffffffffULL;
/** Bit width reserved for the ticket slot inside a ticket id. */
constexpr uint8 PLDT_TICKET_SLOT_BITS = 20;
/** Mask that extracts a ticket slot from a generation-aware ticket id. */
constexpr uint64 PLDT_TICKET_SLOT_MASK = (1ULL << PLDT_TICKET_SLOT_BITS) - 1;

/** Compatibility marker used by the contract registration machinery. */
struct PLDT2
{
};

/**
 * @brief PulseEditor scheduled game contract.
 * @note All dates are interpreted as UTC `DateAndTime` values supplied by QPI.
 */
struct PLDT : public ContractBase
{
public:
	template<typename T>
	/** Split fixed-capacity matrix that stores every exact/misplaced payout tier. */
	struct TierMatrix
	{
		/** First fixed segment of the tier matrix. */
		Array<T, PLDT_TIER_PREFIX_CAPACITY> prefix;
		/** Remaining fixed segment of the tier matrix. */
		Array<T, PLDT_TIER_SUFFIX_CAPACITY> suffix;

		/** Returns the tier value at a stable compact payout-matrix index. */
		const T& get(const uint16 index) const
		{
			return index < PLDT_TIER_PREFIX_CAPACITY ? prefix.get(index) : suffix.get(index - PLDT_TIER_PREFIX_CAPACITY);
		}

		/** Replaces the tier value while hiding the two-array storage split. */
		void set(const uint16 index, const T value)
		{
			if (index < PLDT_TIER_PREFIX_CAPACITY)
			{
				prefix.set(index, value);
			}
			else
			{
				suffix.set(index - PLDT_TIER_PREFIX_CAPACITY, value);
			}
		}

		/** Returns the total number of addressable exact/misplaced tiers. */
		static constexpr uint16 capacity() { return PLDT_TIER_CAPACITY; }
	};

	using TierWeightMatrix = TierMatrix<uint16>;
	using TierAmountMatrix = TierMatrix<uint64>;

	/** Stable outcomes returned by PulseEditor public functions and procedures. */
	enum class EReturnCode : uint8
	{
		/** The request completed successfully. */
		SUCCESS,
		/** The invocator is not authorized for the requested operation. */
		ACCESS_DENIED,
		/** The game id does not identify the active generation in its slot. */
		INVALID_GAME,
		/** The operation is not permitted in the current lifecycle phase. */
		INVALID_STATE,
		/** One or more configuration values violate supported bounds. */
		INVALID_VALUE,
		/** The submitted code violates length, range, or uniqueness rules. */
		INVALID_DIGITS,
		/** The caller lacks the Qubic or asset balance required. */
		INSUFFICIENT_FUNDS,
		/** The attached invocation reward does not equal the ticket price. */
		TICKET_INVALID_PRICE,
		/** The round or global ticket store has no remaining capacity. */
		TICKET_SOLD_OUT,
		/** The player has reached the per-round ticket allowance. */
		PLAYER_TICKET_LIMIT,
		/** Ticket sales have not opened yet. */
		GAME_NOT_STARTED,
		/** Ticket sales have ended for the round. */
		GAME_CLOSED,
		/** A bounded state ledger cannot accept the operation atomically. */
		STORAGE_FULL,
		/** A Qubic or asset custody transfer failed. */
		TRANSFER_FAILED,
		/** The requested round does not exist for the game. */
		INVALID_ROUND,
		/** The ticket id is absent or belongs to an obsolete slot generation. */
		INVALID_TICKET,
		/** The result exists conceptually but its details were reclaimed. */
		HISTORY_EXPIRED,
		/** An internal invariant failed without a more specific public code. */
		UNKNOWN_ERROR,
	};

	/** Determines whether a game ends once or schedules successive rounds. */
	enum class EGameMode : uint8
	{
		/** A single funded round that clears its slot after finalization. */
		ONE_SHOT,
		/** A creator-funded game that may schedule successive rounds. */
		PERMANENT,
	};

	/** Determines whether creator fees are paid out or reinvested. */
	enum class ECreatorRevenueMode : uint8
	{
		/** Creator fees are transferred to the owner after the round. */
		PAYOUT,
		/** Creator fees replenish the game's unreserved creator balance. */
		REINVEST,
	};

	/** Reason a game stops instead of scheduling another round. */
	enum class EGameStopReason : uint8
	{
		/** No stop reason or lifecycle action is currently selected. */
		NONE,
		/** The one-shot round reached its terminal outcome. */
		ONE_SHOT_COMPLETE,
		/** The owner requested a stop after the active round. */
		OWNER_REQUESTED,
		/** The permanent game cannot fund its next round. */
		OUT_OF_FUNDS,
		/** The next round would exceed DateAndTime limits. */
		SCHEDULE_EXHAUSTED,
	};

	/** Currency custody mechanism used for tickets and payouts. */
	enum class ECurrencyMode : uint8
	{
		/** Ticket payments and payouts use native Qubic units. */
		QUBIC,
		/** Ticket payments and payouts use managed asset shares. */
		ASSET,
	};

	/** Persisted lifecycle phase of a game round. */
	enum class EGameStatus : uint8
	{
		/** The slot is free and contains no active game generation. */
		EMPTY_SLOT,
		/** The game exists but its sales window has not opened. */
		SCHEDULED,
		/** The current UTC time is inside the ticket-sales window. */
		SELLING,
		/** Sales ended and the round awaits settlement startup. */
		CLOSED,
		/** Tickets are being classified into winning tiers. */
		COUNTING,
		/** Winner payouts are being transferred incrementally. */
		PAYING,
		/** Terminal transfers and result publication are in progress. */
		FINALIZING,
	};

	/** Outcome recorded when a round reaches finalization. */
	enum class EGameTerminalReason : uint8
	{
		/** No stop reason or lifecycle action is currently selected. */
		NONE,
		/** All winner payouts for the round were completed. */
		SETTLED,
		/** The round closed without an accepted ticket. */
		NO_TICKETS,
		/** No ticket qualified for a configured payout tier. */
		NO_WINNERS,
		/** The owner cancelled before the first sales window. */
		OWNER_CANCELLED,
	};

	/** Settlement state of a stored ticket. */
	enum class ETicketStatus : uint8
	{
		/** The ticket awaits round classification. */
		ACTIVE,
		/** The ticket did not qualify for a payout. */
		LOST,
		/** The ticket payout was transferred successfully. */
		PAID,
	};

	/** Transition requested by time-based lifecycle evaluation. */
	enum class EGameLifecycleAction : uint8
	{
		/** No stop reason or lifecycle action is currently selected. */
		NONE,
		/** Persist the transition into the selling phase. */
		OPEN_SALES,
		/** Finalize immediately because no tickets were sold. */
		FINALIZE_NO_TICKETS,
		/** Derive winning digits and begin incremental settlement. */
		BEGIN_SETTLEMENT,
	};

	/**
	 * @brief Returns the compact tier index for an `(exact, misplaced)` match pair.
	 * @param exact Digits matched in the correct position.
	 * @param misplaced Correct digits found in another position.
	 * @return Stable matrix index in `0..65`.
	 */
	static constexpr uint16 payoutMatrixIndex(const uint8 exact, const uint8 misplaced)
	{
		return static_cast<uint16>(misplaced + div<uint16>(exact * ((PLDT_MAX_CODE_LENGTH * 2) + 3 - exact), 2));
	}

	/** Complete replace-on-write economics queued for a future permanent round. */
	struct GameEconomics
	{
		/** Price of one ticket in Qubic or configured asset shares. */
		uint64 ticketPrice;
		/** Creator-funded amount placed into each round's prize pool. */
		uint64 creatorPrizeSeed;
		/** Maximum tickets accepted by the round. */
		uint16 ticketLimit;
		/** Maximum tickets one entity may hold in the round. */
		uint16 playerTicketLimit;
		/** Ticket-price percentage reserved for the game creator. */
		uint8 creatorFeePercent;
		/** Selects payout or reinvestment of creator fees. */
		ECreatorRevenueMode creatorRevenueMode;
		/** Whether pending economics contain a complete validated replacement. */
		bit isSet;
	};

	/**
	 * @brief Immutable rules, funding ledgers, and mutable current-round state.
	 * @note A permanent slot is cleared only when the game itself stops.
	 */
	struct Game
	{
		/** Entity authorized to administer the game or referenced asset position. */
		id owner;
		/** Asset issuances of which holding any one grants bonus weighting. */
		Array<Asset, PLDT_MAX_BONUS_ASSETS> bonusAssets;
		/** Asset used as game currency when currencyMode is ASSET. */
		Asset currencyAsset;
		/** Validated economics applied only when the next permanent round starts. */
		GameEconomics pendingEconomics;
		/** UTC instant at which ticket sales open. */
		DateAndTime startAt;
		/** UTC instant at which sales close and drawing may begin. */
		DateAndTime drawAt;
		/** Generation-aware identifier of a game slot. */
		uint64 gameId;
		/** Price of one ticket in Qubic or configured asset shares. */
		uint64 ticketPrice;
		/** Creator-funded amount placed into each round's prize pool. */
		uint64 creatorPrizeSeed;
		/** Current round amount reserved exclusively for winner payouts or return. */
		uint64 prizePool;
		/** Creator-fee amount accrued during the current round. */
		uint64 creatorRevenue;
		/** Gross accepted ticket payments for the current round. */
		uint64 totalRevenue;
		/** Winner payouts successfully transferred in the current round. */
		uint64 totalPaid;
		/** Qubic automation fee charged for the current round. */
		uint64 roundFeeSnapshot;
		/** Qubic ledger left after the current round fee; permanent games may spend it on later rounds. */
		uint64 runCredit;
		/** Unreserved game currency available for later seeds or terminal return. */
		uint64 creatorBalance;
		/** Creator-fee currency waiting for resumable owner transfer. */
		uint64 pendingCreatorCurrencyPayout;
		/** Unused creator-balance currency waiting for stop transfer. */
		uint64 pendingCreatorBalancePayout;
		/** Unused Qubic run credit waiting for stop transfer. */
		uint64 pendingRunCreditPayout;
		/** Fixed start-to-draw interval reused by permanent rounds. */
		uint64 roundDurationMicroseconds;
		/** One-based round sequence within a game generation. */
		uint64 roundNumber;
		/** One-based link to the first ticket in this round's chain. */
		uint64 firstTicketLink;
		/** One-based link to the last ticket in this round's chain. */
		uint64 lastTicketLink;
		/** Winner weight applied to bonus-qualified tickets, in basis points. */
		uint32 bonusMultiplierBps;
		/** Prize-pool weights for every exact/misplaced tier, in basis points. */
		TierWeightMatrix tierWeightsBps;
		/** Maximum tickets accepted by the round. */
		uint16 ticketLimit;
		/** Maximum tickets one entity may hold in the round. */
		uint16 playerTicketLimit;
		/** Number of tickets accepted for the current request or round. */
		uint16 ticketCount;
		/** Number of tickets assigned a non-zero payout. */
		uint16 winnerCount;
		/** Number of configured entries in bonusAssets. */
		uint16 bonusAssetCount;
		/** Contract index required to manage currency-asset ownership records. */
		uint16 ownershipManagingContractIndex;
		/** Contract index required to manage currency-asset possession records. */
		uint16 possessionManagingContractIndex;
		/** Ownership manager used when checking bonus-asset holdings. */
		uint16 bonusOwnershipManagingContractIndex;
		/** Possession manager used when checking bonus-asset holdings. */
		uint16 bonusPossessionManagingContractIndex;
		/** One-based link to the matching asset accrual bucket. */
		uint16 assetAccountingLink;
		/** Human-readable game name stored as fixed bytes. */
		Array<uint8, 32> name;
		/** Number of meaningful digits in every code for this game. */
		uint8 codeLength;
		/** Largest permitted digit value, inclusive. */
		uint8 maxDigit;
		/** Ticket-price percentage reserved for the game creator. */
		uint8 creatorFeePercent;
		/** Selects native Qubic custody or managed asset-share custody. */
		ECurrencyMode currencyMode;
		/** Game lifetime model: one-shot or permanent. */
		EGameMode mode;
		/** Selects payout or reinvestment of creator fees. */
		ECreatorRevenueMode creatorRevenueMode;
		/** Round outcome being processed by resumable finalization. */
		EGameTerminalReason finalizingRoundReason;
		/** Stop outcome being processed by resumable finalization. */
		EGameStopReason finalizingStopReason;
		/** Whether a code may contain the same digit more than once. */
		bit allowRepeatedDigits;
		/** Whether the owner requested stopping after the active round. */
		bit stopRequested;
		/** Current persisted lifecycle or oracle status. */
		EGameStatus status;
	};

	/** Generation-aware player entry and its eventual settlement result. */
	struct Ticket
	{
		/** Entity that owns the ticket or is evaluated for bonus eligibility. */
		id player;
		/** Generation-aware identifier of a game slot. */
		uint64 gameId;
		/** Generation-aware identifier of a ticket slot. */
		uint64 ticketId;
		/** One-based link to the next ticket or zero at the chain end. */
		uint64 nextLink;
		/** Currency amount assigned or transferred to the current ticket. */
		uint64 payout;
		/** Basis-point weight of the current winner within its tier. */
		uint32 winnerWeight;
		/** Compact exact/misplaced match-tier index for a ticket. */
		uint16 tierIndex;
		/** Submitted, generated, or returned code digits; only codeLength entries are meaningful. */
		Array<uint8, PLDT_DIGITS_ALIGNED> digits;
		/** Digits that match the winning code in both value and position. */
		uint8 exact;
		/** Winning digit values found in a different position. */
		uint8 misplaced;
		/** Whether this ticket receives the configured bonus winner weight. */
		bit bonusQualified;
		/** Current persisted lifecycle or oracle status. */
		ETicketStatus status;
	};

	/** Published round summary retained in the bounded result history. */
	struct GameResult
	{
		/** Entity authorized to administer the game or referenced asset position. */
		id owner;
		/** Asset used as game currency when currencyMode is ASSET. */
		Asset currencyAsset;
		/** UTC instant at which ticket sales open. */
		DateAndTime startAt;
		/** UTC instant at which sales close and drawing may begin. */
		DateAndTime drawAt;
		/** Generation-aware identifier of a game slot. */
		uint64 gameId;
		/** Current round amount reserved exclusively for winner payouts or return. */
		uint64 prizePool;
		/** Winner payouts successfully transferred in the current round. */
		uint64 totalPaid;
		/** One-based link to the first ticket in this round's chain. */
		uint64 firstTicketLink;
		/** One-based round sequence within a game generation. */
		uint64 roundNumber;
		/** Monotonic publication sequence assigned to this result. */
		uint64 resultSequence;
		/** Tick at which the round result was finalized. */
		uint32 settledTick;
		/** Prize-pool weights for every exact/misplaced tier, in basis points. */
		TierWeightMatrix tierWeightsBps;
		/** Number of tickets accepted for the current request or round. */
		uint16 ticketCount;
		/** Number of tickets assigned a non-zero payout. */
		uint16 winnerCount;
		/** Deterministically generated code used to settle the round. */
		Array<uint8, PLDT_DIGITS_ALIGNED> winningDigits;
		/** Number of meaningful digits in every code for this game. */
		uint8 codeLength;
		/** Selects native Qubic custody or managed asset-share custody. */
		ECurrencyMode currencyMode;
		/** Game lifetime model: one-shot or permanent. */
		EGameMode mode;
		/** Published reason the game stopped after this round. */
		EGameStopReason gameStopReason;
		/** Terminal outcome published for the round. */
		EGameTerminalReason terminalReason;
		/** Whether ticket links and full result details remain queryable. */
		bit detailsAvailable;
	};

	/** Persistent cursors and aggregates for budgeted multi-tick settlement. */
	struct SettlementProgress
	{
		/** Number of winning tickets classified into each tier. */
		TierAmountMatrix tierWinnerCount;
		/** Number of bonus-qualified winners classified into each tier. */
		TierAmountMatrix tierBonusCount;
		/** Prize amount assigned to each winning tier. */
		TierAmountMatrix tierPools;
		/** Fractional remainders used to allocate rounding units deterministically. */
		TierAmountMatrix tierFractions;
		/** Winner share before applying bonus weighting. */
		TierAmountMatrix normalPayout;
		/** Additional winner share caused by bonus qualification. */
		TierAmountMatrix bonusPayout;
		/** Undistributed integer units assigned one-by-one to tier winners. */
		TierAmountMatrix tierRemainder;
		/** Next ticket link to process in resumable settlement. */
		uint64 cursorLink;
		/** Immutable pool amount captured when settlement begins. */
		uint64 prizePoolSnapshot;
		/** Prize amount already assigned across winning tiers. */
		uint64 allocatedPool;
		/** Sum of configured weights for tiers that actually have winners. */
		uint32 activeTierBps;
		/** Number of tickets assigned a non-zero payout. */
		uint16 winnerCount;
		/** Deterministically generated code used to settle the round. */
		Array<uint8, PLDT_DIGITS_ALIGNED> winningDigits;
	};

	/** Platform accrual bucket for one asset and management-index tuple. */
	struct AssetPlatformAccounting
	{
		/** Asset issuance whose shares are inspected or transferred. */
		Asset asset;
		/** Unwithdrawn amount owed to the first platform developer. */
		uint64 developer1Accrued;
		/** Unwithdrawn amount owed to the second platform developer. */
		uint64 developer2Accrued;
		/** Unwithdrawn amount reserved for shareholder distribution. */
		uint64 dividendAccrued;
		/** Total game slots currently occupied. */
		uint16 activeGameCount;
		/** Contract index required to manage currency-asset ownership records. */
		uint16 ownershipManagingContractIndex;
		/** Contract index required to manage currency-asset possession records. */
		uint16 possessionManagingContractIndex;
		/** Whether this accounting bucket is allocated to an asset tuple. */
		bit isActive;
	};

	/** All consensus-persistent PulseEditor state; field order is ABI-sensitive. */
	struct StateData
	{
		/** Generation-aware game records indexed by game slot. */
		Array<Game, PLDT_MAX_GAMES> games;
		/** Per-game resumable settlement progress. */
		Array<SettlementProgress, PLDT_MAX_GAMES> settlements;
		/** Per-slot generation counters used to create fresh game ids. */
		Array<uint64, PLDT_MAX_GAMES> generations;
		/** Generation-aware ticket records shared by all games. */
		Array<Ticket, PLDT_MAX_TICKETS> tickets;
		/** Bounded ring of published round results. */
		Array<GameResult, PLDT_RESULT_STORAGE_SIZE> results;
		/** Working copy of the game currency asset's accrual bucket. */
		Array<AssetPlatformAccounting, PLDT_MAX_GAMES> assetAccounting;
		/** Entity authorized to change platform configuration and withdraw accruals. */
		id platformOwner;
		/** First configured recipient of platform developer fees. */
		id developer1;
		/** Second configured recipient of platform developer fees. */
		id developer2;
		/** Unwithdrawn amount owed to the first platform developer. */
		uint64 developer1Accrued;
		/** Unwithdrawn amount owed to the second platform developer. */
		uint64 developer2Accrued;
		/** Unwithdrawn amount reserved for shareholder distribution. */
		uint64 dividendAccrued;
		/** Qubic charged from run credit for each automated round. */
		uint64 roundFee;
		/** Number of tickets accepted for the current request or round. */
		uint64 ticketCount;
		/** First ticket slot that has never been allocated, or zero before initialization. */
		uint64 nextUnusedTicketSlot;
		/** Head of the free-list of reusable ticket slots. */
		uint64 freeTicketHead;
		/** Number of ticket slots currently available through the free list. */
		uint64 freeTicketCount;
		/** Oldest result sequence still requiring ticket reclamation. */
		uint64 reclaimResultCounter;
		/** Next ticket link awaiting background reclamation. */
		uint64 reclaimTicketLink;
		/** Monotonic sequence used to order and place published results. */
		uint64 resultCounter;
		/** Total game slots currently occupied. */
		uint16 activeGameCount;
		/** Next storage slot from which allocation search resumes. */
		uint16 allocationCursor;
		/** Next game slot from which the periodic scan resumes. */
		uint16 automationCursor;
		/** Ticket-price percentage reserved for platform recipients. */
		uint8 platformFeePercent;
		/** Owner-configurable upper bound for creatorFeePercent. */
		uint8 maxCreatorFeePercent;
		/** Whether background cleanup is traversing this result's ticket chain. */
		bit reclaimingTickets;
	};

	/** Validated data consumed by the create game operation. */
	struct CreateGame_input
	{
		/** Prize-pool weights for every exact/misplaced tier, in basis points. */
		TierWeightMatrix tierWeightsBps;
		/** Asset issuances of which holding any one grants bonus weighting. */
		Array<Asset, PLDT_MAX_BONUS_ASSETS> bonusAssets;
		/** Human-readable game name stored as fixed bytes. */
		Array<uint8, 32> name;
		/** Asset used as game currency when currencyMode is ASSET. */
		Asset currencyAsset;
		/** UTC instant at which ticket sales open. */
		DateAndTime startAt;
		/** UTC instant at which sales close and drawing may begin. */
		DateAndTime drawAt;
		/** Price of one ticket in Qubic or configured asset shares. */
		uint64 ticketPrice;
		/** Creator-funded amount placed into each round's prize pool. */
		uint64 creatorPrizeSeed;
		/** Qubic supplied for the first round fee and optional future rounds. */
		uint64 initialRunCredit;
		/** Game currency supplied for the first prize seed and unreserved creator balance. */
		uint64 initialCreatorBalance;
		/** Winner weight applied to bonus-qualified tickets, in basis points. */
		uint32 bonusMultiplierBps;
		/** Maximum tickets accepted by the round. */
		uint16 ticketLimit;
		/** Maximum tickets one entity may hold in the round. */
		uint16 playerTicketLimit;
		/** Number of configured entries in bonusAssets. */
		uint16 bonusAssetCount;
		/** Contract index required to manage currency-asset ownership records. */
		uint16 ownershipManagingContractIndex;
		/** Contract index required to manage currency-asset possession records. */
		uint16 possessionManagingContractIndex;
		/** Ownership manager used when checking bonus-asset holdings. */
		uint16 bonusOwnershipManagingContractIndex;
		/** Possession manager used when checking bonus-asset holdings. */
		uint16 bonusPossessionManagingContractIndex;
		/** Number of meaningful digits in every code for this game. */
		uint8 codeLength;
		/** Largest permitted digit value, inclusive. */
		uint8 maxDigit;
		/** Ticket-price percentage reserved for the game creator. */
		uint8 creatorFeePercent;
		/** Selects native Qubic custody or managed asset-share custody. */
		ECurrencyMode currencyMode;
		/** Selects whether the game closes or starts another round after settlement. */
		EGameMode mode;
		/** Selects payout or reinvestment of creator fees. */
		ECreatorRevenueMode creatorRevenueMode;
		/** Whether a code may contain the same digit more than once. */
		bit allowRepeatedDigits;
	};

	/** Result data produced by the create game operation. */
	struct CreateGame_output
	{
		/** Generation-aware identifier of a game slot. */
		uint64 gameId;
		/** Zero-based game, ticket, result, or accounting slot under operation. */
		uint16 slot;
		/** Public outcome describing success or the reason no state change occurred. */
		EReturnCode returnCode;
	};

	/** Validated data consumed by the fund game operation. */
	struct FundGame_input
	{
		/** Generation-aware identifier of a game slot. */
		uint64 gameId;
		/** Additional Qubic added to the game run-credit ledger. */
		uint64 runCreditTopUp;
		/** Additional game currency added to the creator-balance ledger. */
		uint64 creatorBalanceTopUp;
	};
	/** Result data produced by the fund game operation. */
	struct FundGame_output
	{
		/** Public outcome describing success or the reason no state change occurred. */
		EReturnCode returnCode;
	};

	/** Validated data consumed by the update game economics operation. */
	struct UpdateGameEconomics_input
	{
		/** Generation-aware identifier of a game slot. */
		uint64 gameId;
		/** Price of one ticket in Qubic or configured asset shares. */
		uint64 ticketPrice;
		/** Creator-funded amount placed into each round's prize pool. */
		uint64 creatorPrizeSeed;
		/** Maximum tickets accepted by the round. */
		uint16 ticketLimit;
		/** Maximum tickets one entity may hold in the round. */
		uint16 playerTicketLimit;
		/** Ticket-price percentage reserved for the game creator. */
		uint8 creatorFeePercent;
		/** Selects payout or reinvestment of creator fees. */
		ECreatorRevenueMode creatorRevenueMode;
	};
	/** Result data produced by the update game economics operation. */
	struct UpdateGameEconomics_output
	{
		/** Public outcome describing success or the reason no state change occurred. */
		EReturnCode returnCode;
	};

	/** Validated data consumed by the stop game operation. */
	struct StopGame_input
	{
		/** Generation-aware identifier of a game slot. */
		uint64 gameId;
	};
	/** Result data produced by the stop game operation. */
	struct StopGame_output
	{
		/** Public outcome describing success or the reason no state change occurred. */
		EReturnCode returnCode;
	};

	/** Validated data consumed by the withdraw game balance operation. */
	struct WithdrawGameBalance_input
	{
		/** Generation-aware identifier of a game slot. */
		uint64 gameId;
		/** Run credit requested for withdrawal. */
		uint64 runCreditAmount;
		/** Creator balance requested for withdrawal. */
		uint64 creatorBalanceAmount;
	};
	/** Result data produced by the withdraw game balance operation. */
	struct WithdrawGameBalance_output
	{
		/** Run credit successfully returned to the owner. */
		uint64 runCreditPaid;
		/** Creator balance successfully returned to the owner. */
		uint64 creatorBalancePaid;
		/** Public outcome describing success or the reason no state change occurred. */
		EReturnCode returnCode;
	};

	/** Validated data consumed by the buy ticket operation. */
	struct BuyTicket_input
	{
		/** Submitted, generated, or returned code digits; only codeLength entries are meaningful. */
		Array<uint8, PLDT_DIGITS_ALIGNED> digits;
		/** Generation-aware identifier of a game slot. */
		uint64 gameId;
	};

	/** Result data produced by the buy ticket operation. */
	struct BuyTicket_output
	{
		/** Generation-aware identifier of a ticket slot. */
		uint64 ticketId;
		/** Compatibility zero-based ticket slot returned to legacy clients. */
		uint64 ticketIndex;
		/** Per-ticket amount added to the winner prize pool. */
		uint64 prizeContribution;
		/** Public outcome describing success or the reason no state change occurred. */
		EReturnCode returnCode;
	};

	/** Validated data consumed by the get game operation. */
	struct GetGame_input
	{
		/** Generation-aware identifier of a game slot. */
		uint64 gameId;
	};
	/** Result data produced by the get game operation. */
	struct GetGame_output
	{
		/** Working copy or returned snapshot of a game record. */
		Game game;
		/** Public outcome describing success or the reason no state change occurred. */
		EReturnCode returnCode;
	};

	/** Composite identity of one immutable game round. */
	struct RoundKey
	{
		/** Generation-aware identifier of a game slot. */
		uint64 gameId;
		/** One-based round sequence within a game generation. */
		uint64 roundNumber;
	};
	using RoundResult = GameResult;
	/** Validated data consumed by the get round result operation. */
	struct GetRoundResult_input
	{
		/** Game and round pair used to identify immutable result history. */
		RoundKey roundKey;
	};
	/** Result data produced by the get round result operation. */
	struct GetRoundResult_output
	{
		/** Published result associated with the requested round. */
		RoundResult roundResult;
		/** Published result associated with the requested game. */
		GameResult gameResult;
		/** Public outcome describing success or the reason no state change occurred. */
		EReturnCode returnCode;
	};
	using GetGameResult_input = GetRoundResult_input;
	using GetGameResult_output = GetRoundResult_output;

	/** Validated data consumed by the get ticket operation. */
	struct GetTicket_input
	{
		/** Generation-aware identifier of a ticket slot. */
		uint64 ticketId;
	};
	/** Result data produced by the get ticket operation. */
	struct GetTicket_output
	{
		/** Working copy or returned snapshot of a ticket record. */
		Ticket ticket;
		/** Public outcome describing success or the reason no state change occurred. */
		EReturnCode returnCode;
	};

	/** Validated data consumed by the validate digits operation. */
	struct ValidateDigits_input
	{
		/** Submitted, generated, or returned code digits; only codeLength entries are meaningful. */
		Array<uint8, PLDT_DIGITS_ALIGNED> digits;
		/** Number of meaningful digits in every code for this game. */
		uint8 codeLength;
		/** Largest permitted digit value, inclusive. */
		uint8 maxDigit;
		/** Whether a code may contain the same digit more than once. */
		bit allowRepeatedDigits;
	};
	/** Result data produced by the validate digits operation. */
	struct ValidateDigits_output
	{
		/** Public outcome describing success or the reason no state change occurred. */
		EReturnCode returnCode;
	};

	using PreviewGame_input = CreateGame_input;
	/** Result data produced by the preview game operation. */
	struct PreviewGame_output
	{
		/** Qubic charged from run credit for each automated round. */
		uint64 roundFee;
		/** Invocation reward required to create the game. */
		uint64 initialQubicRequired;
		/** Asset shares that must be transferred for initial creator funding. */
		uint64 initialCreatorAssetRequired;
		/** Per-ticket amount split between developers and shareholders. */
		uint64 platformFee;
		/** Per-ticket amount accrued to the game creator. */
		uint64 creatorFee;
		/** Per-ticket amount removed from circulation. */
		uint64 burn;
		/** Per-ticket amount added to the winner prize pool. */
		uint64 prizeContribution;
		/** Public outcome describing success or the reason no state change occurred. */
		EReturnCode returnCode;
	};

	/** Validated data consumed by the get platform accounting operation. */
	struct GetPlatformAccounting_input
	{
	};
	/** Result data produced by the get platform accounting operation. */
	struct GetPlatformAccounting_output
	{
		/** Entity authorized to change platform configuration and withdraw accruals. */
		id platformOwner;
		/** First configured recipient of platform developer fees. */
		id developer1;
		/** Second configured recipient of platform developer fees. */
		id developer2;
		/** Unwithdrawn amount owed to the first platform developer. */
		uint64 developer1Accrued;
		/** Unwithdrawn amount owed to the second platform developer. */
		uint64 developer2Accrued;
		/** Unwithdrawn amount reserved for shareholder distribution. */
		uint64 dividendAccrued;
		/** Qubic charged from run credit for each automated round. */
		uint64 roundFee;
		/** Number of tickets accepted for the current request or round. */
		uint64 ticketCount;
		/** Monotonic sequence used to order and place published results. */
		uint64 resultCounter;
		/** Total game slots currently occupied. */
		uint16 activeGameCount;
		/** Ticket-price percentage reserved for platform recipients. */
		uint8 platformFeePercent;
		/** Owner-configurable upper bound for creatorFeePercent. */
		uint8 maxCreatorFeePercent;
	};

	/** Validated data consumed by the set platform config operation. */
	struct SetPlatformConfig_input
	{
		/** Entity authorized to change platform configuration and withdraw accruals. */
		id platformOwner;
		/** First configured recipient of platform developer fees. */
		id developer1;
		/** Second configured recipient of platform developer fees. */
		id developer2;
		/** Qubic charged from run credit for each automated round. */
		uint64 roundFee;
		/** Owner-configurable upper bound for creatorFeePercent. */
		uint8 maxCreatorFeePercent;
	};
	/** Result data produced by the set platform config operation. */
	struct SetPlatformConfig_output
	{
		/** Public outcome describing success or the reason no state change occurred. */
		EReturnCode returnCode;
	};

	/** Validated data consumed by the withdraw platform revenue operation. */
	struct WithdrawPlatformRevenue_input
	{
	};
	/** Result data produced by the withdraw platform revenue operation. */
	struct WithdrawPlatformRevenue_output
	{
		/** Amount successfully transferred to the first developer. */
		uint64 developer1Paid;
		/** Amount successfully transferred to the second developer. */
		uint64 developer2Paid;
		/** Amount successfully distributed to shareholders. */
		uint64 dividendPaid;
		/** Public outcome describing success or the reason no state change occurred. */
		EReturnCode returnCode;
	};

	/** Validated data consumed by the buy tickets operation. */
	struct BuyTickets_input
	{
		/** Generation-aware ticket records shared by all games. */
		Array<Array<uint8, PLDT_DIGITS_ALIGNED>, PLDT_MAX_BATCH_TICKETS> tickets;
		/** Generation-aware identifier of a game slot. */
		uint64 gameId;
		/** Number of tickets accepted for the current request or round. */
		uint16 ticketCount;
	};
	/** Result data produced by the buy tickets operation. */
	struct BuyTickets_output
	{
		/** Generation-aware ids of tickets accepted by a batch purchase. */
		Array<uint64, PLDT_MAX_BATCH_TICKETS> ticketIds;
		/** Compatibility ticket slots accepted by a batch purchase. */
		Array<uint64, PLDT_MAX_BATCH_TICKETS> ticketIndexes;
		/** Number of tickets committed by the batch request. */
		uint16 acceptedCount;
		/** Public outcome describing success or the reason no state change occurred. */
		EReturnCode returnCode;
	};

	/** Validated data consumed by the get player tickets operation. */
	struct GetPlayerTickets_input
	{
		/** Entity that owns the ticket or is evaluated for bonus eligibility. */
		id player;
		/** Game and round pair used to identify immutable result history. */
		RoundKey roundKey;
		/** Zero-based number of matching records to skip. */
		uint64 offset;
		/** Maximum number of records requested in the response page. */
		uint16 limit;
	};
	/** Result data produced by the get player tickets operation. */
	struct GetPlayerTickets_output
	{
		/** Generation-aware ids of tickets accepted by a batch purchase. */
		Array<uint64, 256> ticketIds;
		/** Compatibility ticket slots accepted by a batch purchase. */
		Array<uint64, 256> ticketIndexes;
		/** Aggregate count accumulated across the current operation. */
		uint64 totalCount;
		/** Number of valid records written into the paged response. */
		uint16 returnedCount;
		/** Public outcome describing success or the reason no state change occurred. */
		EReturnCode returnCode;
	};

	/** Validated data consumed by the get games operation. */
	struct GetGames_input
	{
		/** Zero-based number of matching records to skip. */
		uint16 offset;
		/** Maximum number of records requested in the response page. */
		uint16 limit;
	};
	/** Result data produced by the get games operation. */
	struct GetGames_output
	{
		/** Page of generation-aware game identifiers returned to the caller. */
		Array<uint64, 64> gameIds;
		/** Total active records observed while scanning bounded storage. */
		uint16 totalActive;
		/** Number of valid records written into the paged response. */
		uint16 returnedCount;
		/** Public outcome describing success or the reason no state change occurred. */
		EReturnCode returnCode;
	};

	/** Validated data consumed by the get winners operation. */
	struct GetWinners_input
	{
		/** Game and round pair used to identify immutable result history. */
		RoundKey roundKey;
		/** Zero-based number of matching records to skip. */
		uint64 offset;
		/** Maximum number of records requested in the response page. */
		uint16 limit;
	};
	/** Result data produced by the get winners operation. */
	struct GetWinners_output
	{
		/** Generation-aware ids of tickets accepted by a batch purchase. */
		Array<uint64, 256> ticketIds;
		/** Compatibility ticket slots accepted by a batch purchase. */
		Array<uint64, 256> ticketIndexes;
		/** Aggregate count accumulated across the current operation. */
		uint64 totalCount;
		/** Number of valid records written into the paged response. */
		uint16 returnedCount;
		/** Public outcome describing success or the reason no state change occurred. */
		EReturnCode returnCode;
	};

	/** Validated data consumed by the transfer share management rights operation. */
	struct TransferShareManagementRights_input
	{
		/** Asset issuance whose shares are inspected or transferred. */
		Asset asset;
		/** Managed asset shares requested for rights transfer. */
		sint64 numberOfShares;
		/** Contract index that should receive share-management rights. */
		uint16 newManagingContractIndex;
	};
	/** Result data produced by the transfer share management rights operation. */
	struct TransferShareManagementRights_output
	{
		/** QPI transfer result; a negative value indicates failure. */
		sint64 transferResult;
		/** Public outcome describing success or the reason no state change occurred. */
		EReturnCode returnCode;
	};

	/** Validated data consumed by the withdraw asset platform revenue operation. */
	struct WithdrawAssetPlatformRevenue_input
	{
		/** Asset issuance whose shares are inspected or transferred. */
		Asset asset;
		/** Contract index required to manage currency-asset ownership records. */
		uint16 ownershipManagingContractIndex;
		/** Contract index required to manage currency-asset possession records. */
		uint16 possessionManagingContractIndex;
	};
	/** Result data produced by the withdraw asset platform revenue operation. */
	struct WithdrawAssetPlatformRevenue_output
	{
		/** Amount successfully transferred to the first developer. */
		uint64 developer1Paid;
		/** Amount successfully transferred to the second developer. */
		uint64 developer2Paid;
		/** Amount successfully distributed to shareholders. */
		uint64 dividendPaid;
		/** Public outcome describing success or the reason no state change occurred. */
		EReturnCode returnCode;
	};
	/** Validated data consumed by the transfer asset dividend operation. */
	struct TransferAssetDividend_input
	{
		/** Asset issuance distributed proportionally to PLDT shareholders. */
		Asset dividendAsset;
		/** Asset units available for proportional shareholder distribution. */
		uint64 dividendAmount;
	};
	/** Result data produced by the transfer asset dividend operation. */
	struct TransferAssetDividend_output
	{
		/** Asset units already transferred during proportional shareholder distribution. */
		uint64 distributedAmount;
		/** Whether any resumable transfer attempted in this operation failed. */
		bit failed;
	};
	/** QPI scratch state for transfer asset dividend; contract routines cannot declare stack locals. */
	struct TransferAssetDividend_locals
	{
		/** Iterator state for bounded PLDT shareholder distribution. */
		AssetPossessionIterator shareholdersIter;
		/** Contract-share asset whose holders receive asset dividends. */
		Asset shareholdersAsset;
		/** Cumulative prize amount that should be allocated at this step. */
		uint64 targetDistribution;
		/** Whole currency units still unallocated after integer division. */
		uint64 remainder;
		/** Unassigned asset units after whole-share distribution. */
		uint64 holderRemainder;
		/** Whole asset units assigned per held PLDT share. */
		sint64 dividendPerShare;
		/** PLDT shares possessed by the current shareholder. */
		sint64 holderShares;
		/** Asset units assigned to the current PLDT shareholder. */
		sint64 holderDividend;
		/** QPI transfer result; a negative value indicates failure. */
		sint64 transferResult;
	};
	/** QPI scratch state for withdraw asset platform revenue; contract routines cannot declare stack locals. */
	struct WithdrawAssetPlatformRevenue_locals
	{
		/** Request passed to the dividend helper. */
		TransferAssetDividend_input dividendInput;
		/** Response returned by the dividend helper. */
		TransferAssetDividend_output dividendOutput;
		/** Working copy of one asset platform-accrual bucket. */
		AssetPlatformAccounting accounting;
		/** Loop cursor for the bounded collection being processed. */
		uint64 i;
		/** QPI transfer result; a negative value indicates failure. */
		sint64 transferResult;
		/** Whether the requested record was found during bounded lookup. */
		bit found;
		/** Whether any resumable transfer attempted in this operation failed. */
		bit failed;
	};

	/** Validated data consumed by the count matches operation. */
	struct CountMatches_input
	{
		/** Working copy of the player's submitted code. */
		Array<uint8, PLDT_DIGITS_ALIGNED> playerDigits;
		/** Deterministically generated code used to settle the round. */
		Array<uint8, PLDT_DIGITS_ALIGNED> winningDigits;
		/** Number of meaningful digits in every code for this game. */
		uint8 codeLength;
	};
	/** Result data produced by the count matches operation. */
	struct CountMatches_output
	{
		/** Compact exact/misplaced match-tier index for a ticket. */
		uint16 tierIndex;
		/** Digits that match the winning code in both value and position. */
		uint8 exact;
		/** Winning digit values found in a different position. */
		uint8 misplaced;
	};
	/** QPI scratch state for count matches; contract routines cannot declare stack locals. */
	struct CountMatches_locals
	{
		/** Per-player ticket counts used during settlement accounting. */
		Array<uint8, PLDT_DIGIT_BUCKETS> playerCounts;
		/** Per-tier winner counts used to divide the prize pool. */
		Array<uint8, PLDT_DIGIT_BUCKETS> winningCounts;
		/** Loop cursor for the bounded collection being processed. */
		uint64 i;
		/** Current submitted digit being compared. */
		uint8 playerDigit;
		/** Current winning digit being compared. */
		uint8 winningDigit;
		/** Number of distinct players represented in the round. */
		uint8 playerCount;
		/** Number of matching digits classified for one ticket. */
		uint8 winningCount;
	};

	/** Validated data consumed by the generate winning digits operation. */
	struct GenerateWinningDigits_input
	{
		/** Deterministic hash-derived seed used to generate winning digits. */
		uint64 seed;
		/** Number of meaningful digits in every code for this game. */
		uint8 codeLength;
		/** Largest permitted digit value, inclusive. */
		uint8 maxDigit;
		/** Whether a code may contain the same digit more than once. */
		bit allowRepeatedDigits;
	};
	/** Result data produced by the generate winning digits operation. */
	struct GenerateWinningDigits_output
	{
		/** Submitted, generated, or returned code digits; only codeLength entries are meaningful. */
		Array<uint8, PLDT_DIGITS_ALIGNED> digits;
	};
	/** QPI scratch state for generate winning digits; contract routines cannot declare stack locals. */
	struct GenerateWinningDigits_locals
	{
		/** Presence table marking digits already selected or matched. */
		Array<uint8, PLDT_DIGIT_BUCKETS> used;
		/** Candidate digit or scalar value being validated. */
		uint64 value;
		/** Current zero-based storage or array position. */
		uint64 index;
		/** Collision retries consumed while generating a unique digit. */
		uint8 attempts;
		/** Candidate winning digit before uniqueness is confirmed. */
		uint8 candidate;
		/** Deterministic replacement digit used after retry exhaustion. */
		uint8 fallback;
	};

	/** QPI scratch state for validate digits; contract routines cannot declare stack locals. */
	struct ValidateDigits_locals
	{
		/** Presence table used to reject repeated submitted digits. */
		Array<uint8, PLDT_DIGIT_BUCKETS> seen;
		/** Loop cursor for the bounded collection being processed. */
		uint64 i;
		/** Current digit being validated or inserted into a presence table. */
		uint8 digit;
	};

	/** Validated data consumed by the calculate ticket economics operation. */
	struct CalculateTicketEconomics_input
	{
		/** Price of one ticket in Qubic or configured asset shares. */
		uint64 ticketPrice;
		/** Ticket-price percentage reserved for the game creator. */
		uint8 creatorFeePercent;
		/** Ticket-price percentage reserved for platform recipients. */
		uint8 platformFeePercent;
	};
	/** Result data produced by the calculate ticket economics operation. */
	struct CalculateTicketEconomics_output
	{
		/** Per-ticket amount split between developers and shareholders. */
		uint64 platformFee;
		/** Ticket price remaining after creator, platform, and burn deductions. */
		uint64 net;
		/** Per-ticket amount accrued to the game creator. */
		uint64 creatorFee;
		/** Per-ticket amount removed from circulation. */
		uint64 burn;
		/** Per-ticket amount added to the winner prize pool. */
		uint64 prizeContribution;
		/** Amount accrued to the first platform developer. */
		uint64 developer1Fee;
		/** Amount accrued to the second platform developer. */
		uint64 developer2Fee;
		/** Amount accrued for PLDT shareholder distribution. */
		uint64 dividendFee;
	};

	/** Validated data consumed by the evaluate bonus qualification operation. */
	struct EvaluateBonusQualification_input
	{
		/** Working copy or returned snapshot of a game record. */
		Game game;
		/** Entity that owns the ticket or is evaluated for bonus eligibility. */
		id player;
	};
	/** Result data produced by the evaluate bonus qualification operation. */
	struct EvaluateBonusQualification_output
	{
		/** Whether the player's holdings satisfy any configured bonus asset. */
		bit qualified;
	};
	/** QPI scratch state for evaluate bonus qualification; contract routines cannot declare stack locals. */
	struct EvaluateBonusQualification_locals
	{
		/** Loop cursor for the bounded collection being processed. */
		uint64 i;
		/** Managed asset shares currently possessed by the inspected entity. */
		sint64 possessedShares;
	};

	/** Validated data consumed by the evaluate game lifecycle operation. */
	struct EvaluateGameLifecycle_input
	{
		/** Working copy or returned snapshot of a game record. */
		Game game;
		/** Current UTC time obtained from QPI for lifecycle decisions. */
		DateAndTime now;
	};
	/** Result data produced by the evaluate game lifecycle operation. */
	struct EvaluateGameLifecycle_output
	{
		/** Time-adjusted status used by read and purchase paths. */
		EGameStatus effectiveStatus;
		/** Lifecycle transition selected without mutating state. */
		EGameLifecycleAction action;
		/** Purchase-specific outcome produced by lifecycle evaluation. */
		EReturnCode purchaseReturnCode;
		/** Whether the owner may cancel before the first sales window. */
		bit canCancel;
	};

	/** QPI scratch state for preview game; contract routines cannot declare stack locals. */
	struct PreviewGame_locals
	{
		/** Request passed to the economics helper. */
		CalculateTicketEconomics_input economicsInput;
		/** Response returned by the economics helper. */
		CalculateTicketEconomics_output economicsOutput;
		/** Latest UTC draw allowed by the scheduling horizon. */
		DateAndTime maxDrawAt;
		/** Sum of configured tier weights used to enforce exactly 10000 basis points. */
		uint64 weightTotal;
		/** Creator plus prize amount that may need terminal return per ticket. */
		uint64 refundablePerTicket;
		/** Loop cursor for the bounded collection being processed. */
		uint64 i;
		/** Game-currency shares currently managed by PulseEditor. */
		sint64 managedCurrencyShares;
		/** Digits that match the winning code in both value and position. */
		uint8 exact;
		/** Winning digit values found in a different position. */
		uint8 misplaced;
	};

	/** Validated data consumed by the refund invocation reward operation. */
	struct RefundInvocationReward_input
	{
		/** Public outcome describing success or the reason no state change occurred. */
		EReturnCode returnCode;
	};
	/** Result data produced by the refund invocation reward operation. */
	struct RefundInvocationReward_output
	{
		/** Public outcome describing success or the reason no state change occurred. */
		EReturnCode returnCode;
	};
	/** Validated data consumed by the transfer game currency operation. */
	struct TransferGameCurrency_input
	{
		/** Working copy or returned snapshot of a game record. */
		Game game;
		/** Entity that receives the requested transfer. */
		id destination;
		/** Currency quantity requested for the current transfer. */
		uint64 amount;
	};
	/** Result data produced by the transfer game currency operation. */
	struct TransferGameCurrency_output
	{
		/** QPI transfer result; a negative value indicates failure. */
		sint64 transferResult;
	};
	/** Validated data consumed by the burn collected ticket payment operation. */
	struct BurnCollectedTicketPayment_input
	{
		/** Working copy or returned snapshot of a game record. */
		Game game;
		/** Number of tickets accepted for the current request or round. */
		uint16 ticketCount;
	};
	/** Result data produced by the burn collected ticket payment operation. */
	struct BurnCollectedTicketPayment_output
	{
		/** Public outcome describing success or the reason no state change occurred. */
		EReturnCode returnCode;
	};
	/** QPI scratch state for burn collected ticket payment; contract routines cannot declare stack locals. */
	struct BurnCollectedTicketPayment_locals
	{
		/** Request passed to the transfer helper. */
		TransferGameCurrency_input transferInput;
		/** Response returned by the transfer helper. */
		TransferGameCurrency_output transferOutput;
		/** Request passed to the economics helper. */
		CalculateTicketEconomics_input economicsInput;
		/** Response returned by the economics helper. */
		CalculateTicketEconomics_output economicsOutput;
		/** Aggregate burn amount for all tickets in the accepted purchase. */
		uint64 totalBurn;
	};

	/** Validated data consumed by the apply accepted ticket operation. */
	struct ApplyAcceptedTicket_input
	{
		/** Submitted, generated, or returned code digits; only codeLength entries are meaningful. */
		Array<uint8, PLDT_DIGITS_ALIGNED> digits;
		/** Generation-aware identifier of a game slot. */
		uint64 gameId;
		/** Whether this ticket receives the configured bonus winner weight. */
		bit bonusQualified;
	};
	/** Result data produced by the apply accepted ticket operation. */
	struct ApplyAcceptedTicket_output
	{
		/** Generation-aware identifier of a ticket slot. */
		uint64 ticketId;
		/** Compatibility zero-based ticket slot returned to legacy clients. */
		uint64 ticketIndex;
		/** Per-ticket amount added to the winner prize pool. */
		uint64 prizeContribution;
		/** Public outcome describing success or the reason no state change occurred. */
		EReturnCode returnCode;
	};
	/** QPI scratch state for apply accepted ticket; contract routines cannot declare stack locals. */
	struct ApplyAcceptedTicket_locals
	{
		/** Request passed to the economics helper. */
		CalculateTicketEconomics_input economicsInput;
		/** Response returned by the economics helper. */
		CalculateTicketEconomics_output economicsOutput;
		/** Working copy of the game currency asset's accrual bucket. */
		AssetPlatformAccounting assetAccounting;
		/** Working copy or returned snapshot of a game record. */
		Game game;
		/** Ticket record inspected before linking a newly accepted ticket. */
		Ticket previousTicket;
		/** Working copy or returned snapshot of a ticket record. */
		Ticket ticket;
		/** Zero-based game, ticket, result, or accounting slot under operation. */
		uint16 slot;
		/** Zero-based ticket slot extracted from a ticket id or reclaim cursor. */
		uint64 ticketSlot;
		/** Generation counter preventing stale ticket ids from aliasing reused slots. */
		uint64 ticketGeneration;
	};

	/** QPI scratch state for create game; contract routines cannot declare stack locals. */
	struct CreateGame_locals
	{
		/** Request passed to the refund helper. */
		RefundInvocationReward_input refundInput;
		/** Response returned by the refund helper. */
		RefundInvocationReward_output refundOutput;
		/** Working copy or returned snapshot of a game record. */
		Game game;
		/** Request passed to the preview helper. */
		PreviewGame_input previewInput;
		/** Response returned by the preview helper. */
		PreviewGame_output previewOutput;
		/** Working copy of the game currency asset's accrual bucket. */
		AssetPlatformAccounting assetAccounting;
		/** Latest UTC draw allowed by the scheduling horizon. */
		DateAndTime maxDrawAt;
		/** Generation counter preventing stale game ids from aliasing reused slots. */
		uint64 generation;
		/** Exact Qubic invocation reward required by the operation. */
		uint64 expectedReward;
		/** Amount accrued to the first platform developer. */
		uint64 developer1Fee;
		/** Amount accrued to the second platform developer. */
		uint64 developer2Fee;
		/** Amount accrued for PLDT shareholder distribution. */
		uint64 dividendFee;
		/** Loop cursor for the bounded collection being processed. */
		uint64 i;
		/** Managed asset shares currently possessed by the inspected entity. */
		sint64 possessedShares;
		/** QPI transfer result; a negative value indicates failure. */
		sint64 transferResult;
		/** Zero-based game, ticket, result, or accounting slot under operation. */
		uint16 slot;
		/** Potential free slot being checked before allocation. */
		uint16 candidateSlot;
		/** Zero-based asset accounting slot selected for the game. */
		uint16 accountingSlot;
		/** Per-owner count used to enforce the active-game limit. */
		uint16 creatorActiveGames;
		/** Whether the requested record was found during bounded lookup. */
		bit found;
		/** Whether an existing asset accrual bucket matched the currency tuple. */
		bit accountingFound;
	};

	/** QPI scratch state for fund game; contract routines cannot declare stack locals. */
	struct FundGame_locals
	{
		/** Request passed to the refund helper. */
		RefundInvocationReward_input refundInput;
		/** Response returned by the refund helper. */
		RefundInvocationReward_output refundOutput;
		/** Working copy or returned snapshot of a game record. */
		Game game;
		/** Exact Qubic invocation reward required by the operation. */
		uint64 expectedReward;
		/** Managed asset shares currently possessed by the inspected entity. */
		sint64 possessedShares;
		/** QPI transfer result; a negative value indicates failure. */
		sint64 transferResult;
		/** Zero-based game, ticket, result, or accounting slot under operation. */
		uint16 slot;
	};

	/** QPI scratch state for withdraw game balance; contract routines cannot declare stack locals. */
	struct WithdrawGameBalance_locals
	{
		/** Working copy or returned snapshot of a game record. */
		Game game;
		/** QPI transfer result; a negative value indicates failure. */
		sint64 transferResult;
		/** Zero-based game, ticket, result, or accounting slot under operation. */
		uint16 slot;
		/** Whether any resumable transfer attempted in this operation failed. */
		bit failed;
	};

	/** QPI scratch state for update game economics; contract routines cannot declare stack locals. */
	struct UpdateGameEconomics_locals
	{
		/** Request passed to the economics helper. */
		CalculateTicketEconomics_input economicsInput;
		/** Response returned by the economics helper. */
		CalculateTicketEconomics_output economicsOutput;
		/** Working copy or returned snapshot of a game record. */
		Game game;
		/** Validated next-round economics before it is committed. */
		GameEconomics pending;
		/** Creator plus prize amount that may need terminal return per ticket. */
		uint64 refundablePerTicket;
		/** Zero-based game, ticket, result, or accounting slot under operation. */
		uint16 slot;
	};

	/** QPI scratch state for buy ticket; contract routines cannot declare stack locals. */
	struct BuyTicket_locals
	{
		/** Request passed to the refund helper. */
		RefundInvocationReward_input refundInput;
		/** Response returned by the refund helper. */
		RefundInvocationReward_output refundOutput;
		/** Request passed to the transfer helper. */
		TransferGameCurrency_input transferInput;
		/** Response returned by the transfer helper. */
		TransferGameCurrency_output transferOutput;
		/** Request passed to the apply helper. */
		ApplyAcceptedTicket_input applyInput;
		/** Response returned by the apply helper. */
		ApplyAcceptedTicket_output applyOutput;
		/** Request passed to the burn helper. */
		BurnCollectedTicketPayment_input burnInput;
		/** Response returned by the burn helper. */
		BurnCollectedTicketPayment_output burnOutput;
		/** Request passed to the lifecycle helper. */
		EvaluateGameLifecycle_input lifecycleInput;
		/** Response returned by the lifecycle helper. */
		EvaluateGameLifecycle_output lifecycleOutput;
		/** Request passed to the bonus helper. */
		EvaluateBonusQualification_input bonusInput;
		/** Response returned by the bonus helper. */
		EvaluateBonusQualification_output bonusOutput;
		/** Request passed to the economics helper. */
		CalculateTicketEconomics_input economicsInput;
		/** Response returned by the economics helper. */
		CalculateTicketEconomics_output economicsOutput;
		/** Working copy or returned snapshot of a game record. */
		Game game;
		/** Working copy or returned snapshot of a ticket record. */
		Ticket ticket;
		/** Ticket record inspected before linking a newly accepted ticket. */
		Ticket previousTicket;
		/** Working copy of the game currency asset's accrual bucket. */
		AssetPlatformAccounting assetAccounting;
		/** Request passed to the validate helper. */
		ValidateDigits_input validateInput;
		/** Response returned by the validate helper. */
		ValidateDigits_output validateOutput;
		/** Current UTC time obtained from QPI for lifecycle decisions. */
		DateAndTime now;
		/** Current one-based ticket link during list traversal. */
		uint64 link;
		/** Tickets already owned by the invocator in this round. */
		uint64 playerTickets;
		/** Managed asset shares currently possessed by the inspected entity. */
		sint64 possessedShares;
		/** QPI transfer result; a negative value indicates failure. */
		sint64 transferResult;
		/** Zero-based game, ticket, result, or accounting slot under operation. */
		uint16 slot;
	};

	/** QPI scratch state for buy tickets; contract routines cannot declare stack locals. */
	struct BuyTickets_locals
	{
		/** Request passed to the refund helper. */
		RefundInvocationReward_input refundInput;
		/** Response returned by the refund helper. */
		RefundInvocationReward_output refundOutput;
		/** Request passed to the transfer helper. */
		TransferGameCurrency_input transferInput;
		/** Response returned by the transfer helper. */
		TransferGameCurrency_output transferOutput;
		/** Request passed to the apply helper. */
		ApplyAcceptedTicket_input applyInput;
		/** Response returned by the apply helper. */
		ApplyAcceptedTicket_output applyOutput;
		/** Request passed to the burn helper. */
		BurnCollectedTicketPayment_input burnInput;
		/** Response returned by the burn helper. */
		BurnCollectedTicketPayment_output burnOutput;
		/** Request passed to the lifecycle helper. */
		EvaluateGameLifecycle_input lifecycleInput;
		/** Response returned by the lifecycle helper. */
		EvaluateGameLifecycle_output lifecycleOutput;
		/** Request passed to the bonus helper. */
		EvaluateBonusQualification_input bonusInput;
		/** Response returned by the bonus helper. */
		EvaluateBonusQualification_output bonusOutput;
		/** Request passed to the economics helper. */
		CalculateTicketEconomics_input economicsInput;
		/** Response returned by the economics helper. */
		CalculateTicketEconomics_output economicsOutput;
		/** Request passed to the validate helper. */
		ValidateDigits_input validateInput;
		/** Response returned by the validate helper. */
		ValidateDigits_output validateOutput;
		/** Working copy or returned snapshot of a game record. */
		Game game;
		/** Working copy or returned snapshot of a ticket record. */
		Ticket ticket;
		/** Working copy of the game currency asset's accrual bucket. */
		AssetPlatformAccounting assetAccounting;
		/** Current UTC time obtained from QPI for lifecycle decisions. */
		DateAndTime now;
		/** Gross price of all tickets in the batch. */
		uint64 totalPrice;
		/** Amount accrued to the first platform developer. */
		uint64 developer1Fee;
		/** Amount accrued to the second platform developer. */
		uint64 developer2Fee;
		/** Amount accrued for PLDT shareholder distribution. */
		uint64 dividendFee;
		/** Current one-based ticket link during list traversal. */
		uint64 link;
		/** Tickets already owned by the invocator in this round. */
		uint64 playerTickets;
		/** Loop cursor for the bounded collection being processed. */
		uint64 i;
		/** Managed asset shares currently possessed by the inspected entity. */
		sint64 possessedShares;
		/** QPI transfer result; a negative value indicates failure. */
		sint64 transferResult;
		/** Zero-based game, ticket, result, or accounting slot under operation. */
		uint16 slot;
	};

	/** Validated data consumed by the finalize game operation. */
	struct FinalizeGame_input
	{
		/** Zero-based game, ticket, result, or accounting slot under operation. */
		uint16 slot;
		/** Requested terminal outcome passed to finalization. */
		EGameTerminalReason reason;
	};
	/** Result data produced by the finalize game operation. */
	struct FinalizeGame_output
	{
		/** Public outcome describing success or the reason no state change occurred. */
		EReturnCode returnCode;
	};
	/** QPI scratch state for stop game; contract routines cannot declare stack locals. */
	struct StopGame_locals
	{
		/** Request passed to the finalize helper. */
		FinalizeGame_input finalizeInput;
		/** Response returned by the finalize helper. */
		FinalizeGame_output finalizeOutput;
		/** Working copy or returned snapshot of a game record. */
		Game game;
		/** Zero-based game, ticket, result, or accounting slot under operation. */
		uint16 slot;
	};
	/** Validated data consumed by the clear game slot operation. */
	struct ClearGameSlot_input
	{
		/** Zero-based game, ticket, result, or accounting slot under operation. */
		uint16 slot;
	};
	/** Result data produced by the clear game slot operation. */
	struct ClearGameSlot_output
	{
	};

	/** QPI scratch state for get game; contract routines cannot declare stack locals. */
	struct GetGame_locals
	{
		/** Request passed to the lifecycle helper. */
		EvaluateGameLifecycle_input lifecycleInput;
		/** Response returned by the lifecycle helper. */
		EvaluateGameLifecycle_output lifecycleOutput;
	};

	/** QPI scratch state for get round result; contract routines cannot declare stack locals. */
	struct GetRoundResult_locals
	{
		/** Working copy or returned snapshot of a round result. */
		GameResult result;
		/** Generation-aware identifier of a game slot. */
		uint64 gameId;
		/** One-based round sequence within a game generation. */
		uint64 roundNumber;
		/** Whether the inspected result slot contains a published result. */
		uint64 available;
		/** Bounded scan counter for the current collection. */
		uint64 counter;
		/** Loop cursor for the bounded collection being processed. */
		uint64 i;
		/** Whether any history entry for the requested game was observed. */
		bit foundGame;
	};

	/** Validated data consumed by the find game ticket list operation. */
	struct FindGameTicketList_input
	{
		/** Generation-aware identifier of a game slot. */
		uint64 gameId;
		/** One-based round sequence within a game generation. */
		uint64 roundNumber;
	};
	/** Result data produced by the find game ticket list operation. */
	struct FindGameTicketList_output
	{
		/** One-based link to the first ticket in this round's chain. */
		uint64 firstTicketLink;
		/** Public outcome describing success or the reason no state change occurred. */
		EReturnCode returnCode;
	};
	/** QPI scratch state for find game ticket list; contract routines cannot declare stack locals. */
	struct FindGameTicketList_locals
	{
		/** Working copy or returned snapshot of a round result. */
		GameResult result;
		/** Whether the inspected result slot contains a published result. */
		uint64 available;
		/** Bounded scan counter for the current collection. */
		uint64 counter;
		/** Loop cursor for the bounded collection being processed. */
		uint64 i;
		/** Whether any history entry for the requested game was observed. */
		bit foundGame;
	};

	/** QPI scratch state for get player tickets; contract routines cannot declare stack locals. */
	struct GetPlayerTickets_locals
	{
		/** Request passed to the find helper. */
		FindGameTicketList_input findInput;
		/** Response returned by the find helper. */
		FindGameTicketList_output findOutput;
		/** Working copy or returned snapshot of a ticket record. */
		Ticket ticket;
		/** Current one-based ticket link during list traversal. */
		uint64 link;
		/** Number of matching records skipped to satisfy pagination offset. */
		uint64 skipped;
	};

	/** QPI scratch state for get winners; contract routines cannot declare stack locals. */
	struct GetWinners_locals
	{
		/** Request passed to the find helper. */
		FindGameTicketList_input findInput;
		/** Response returned by the find helper. */
		FindGameTicketList_output findOutput;
		/** Working copy or returned snapshot of a ticket record. */
		Ticket ticket;
		/** Current one-based ticket link during list traversal. */
		uint64 link;
		/** Number of matching records skipped to satisfy pagination offset. */
		uint64 skipped;
	};

	/** QPI scratch state for get games; contract routines cannot declare stack locals. */
	struct GetGames_locals
	{
		/** Working copy or returned snapshot of a game record. */
		Game game;
		/** Number of matching records skipped to satisfy pagination offset. */
		uint64 skipped;
		/** Loop cursor for the bounded collection being processed. */
		uint16 i;
	};

	/** Canonical bytes hashed to derive deterministic winning digits. */
	struct BeginSettlement_randomData
	{
		/** Previous Spectrum digest anchoring deterministic round randomness. */
		m256i prevSpectrumDigest;
		/** Generation-aware identifier of a game slot. */
		uint64 gameId;
		/** One-based round sequence within a game generation. */
		uint64 roundNumber;
		/** Number of tickets accepted for the current request or round. */
		uint16 ticketCount;
	};
	/** Validated data consumed by the begin settlement operation. */
	struct BeginSettlement_input
	{
		/** Zero-based game, ticket, result, or accounting slot under operation. */
		uint16 slot;
	};
	/** Result data produced by the begin settlement operation. */
	struct BeginSettlement_output
	{
		/** Public outcome describing success or the reason no state change occurred. */
		EReturnCode returnCode;
	};
	/** QPI scratch state for begin settlement; contract routines cannot declare stack locals. */
	struct BeginSettlement_locals
	{
		/** Canonical round context assembled before deterministic hashing. */
		BeginSettlement_randomData randomData;
		/** Request passed to the generate helper. */
		GenerateWinningDigits_input generateInput;
		/** Response returned by the generate helper. */
		GenerateWinningDigits_output generateOutput;
		/** Persistent settlement state for the current game slot. */
		SettlementProgress progress;
		/** Working copy or returned snapshot of a game record. */
		Game game;
		/** Deterministic hash-derived seed used to generate winning digits. */
		uint64 seed;
	};

	/** QPI scratch state for finalize game; contract routines cannot declare stack locals. */
	struct FinalizeGame_locals
	{
		/** Request passed to the transfer helper. */
		TransferGameCurrency_input transferInput;
		/** Response returned by the transfer helper. */
		TransferGameCurrency_output transferOutput;
		/** Request passed to the clear helper. */
		ClearGameSlot_input clearInput;
		/** Response returned by the clear helper. */
		ClearGameSlot_output clearOutput;
		/** Persistent settlement state for the current game slot. */
		SettlementProgress progress;
		/** Working copy or returned snapshot of a round result. */
		GameResult result;
		/** Result slot that may need ticket reclamation before reuse. */
		GameResult previousResult;
		/** Working copy or returned snapshot of a game record. */
		Game game;
		/** Computed UTC opening time for the next permanent round. */
		DateAndTime nextStartAt;
		/** Computed UTC draw time for the next permanent round. */
		DateAndTime nextDrawAt;
		/** Ring-buffer slot selected for a round result. */
		uint64 resultIndex;
		/** Amount accrued to the first platform developer. */
		uint64 developer1Fee;
		/** Amount accrued to the second platform developer. */
		uint64 developer2Fee;
		/** Amount accrued for PLDT shareholder distribution. */
		uint64 dividendFee;
		/** Remaining capacity in the destination bounded ledger. */
		uint64 availableCredit;
		/** Amount that fits in the destination bounded ledger. */
		uint64 amountToCredit;
		/** QPI transfer result; a negative value indicates failure. */
		sint64 transferResult;
	};

	/** QPI scratch state for clear game slot; contract routines cannot declare stack locals. */
	struct ClearGameSlot_locals
	{
		/** Working copy of the game currency asset's accrual bucket. */
		AssetPlatformAccounting assetAccounting;
		/** Working copy or returned snapshot of a game record. */
		Game game;
		/** Zeroed replacement used when releasing settlement state. */
		SettlementProgress emptyProgress;
		/** Zeroed replacement used when releasing a game slot. */
		Game emptyGame;
	};

	/** Validated data consumed by the advance settlement operation. */
	struct AdvanceSettlement_input
	{
		/** Maximum state-changing work allowed in this invocation. */
		uint64 actionBudget;
		/** Zero-based game, ticket, result, or accounting slot under operation. */
		uint16 slot;
	};
	/** Result data produced by the advance settlement operation. */
	struct AdvanceSettlement_output
	{
		/** Number of bounded settlement or reclamation actions consumed. */
		uint64 actionsUsed;
		/** Public outcome describing success or the reason no state change occurred. */
		EReturnCode returnCode;
	};
	/** QPI scratch state for advance settlement; contract routines cannot declare stack locals. */
	struct AdvanceSettlement_locals
	{
		/** Request passed to the transfer helper. */
		TransferGameCurrency_input transferInput;
		/** Response returned by the transfer helper. */
		TransferGameCurrency_output transferOutput;
		/** Persistent settlement state for the current game slot. */
		SettlementProgress progress;
		/** Request passed to the match helper. */
		CountMatches_input matchInput;
		/** Response returned by the match helper. */
		CountMatches_output matchOutput;
		/** Request passed to the finalize helper. */
		FinalizeGame_input finalizeInput;
		/** Response returned by the finalize helper. */
		FinalizeGame_output finalizeOutput;
		/** Working copy or returned snapshot of a game record. */
		Game game;
		/** Working copy or returned snapshot of a ticket record. */
		Ticket ticket;
		/** Remaining actions available to the resumable operation. */
		uint64 budget;
		/** Current one-based ticket link during list traversal. */
		uint64 link;
		/** Aggregate count for the tier or collection currently being scanned. */
		uint64 count;
		/** Count of bonus-qualified winners in the current tier. */
		uint64 bonusCount;
		/** Sum of normal and bonus winner weights in the current tier. */
		uint64 totalWeight;
		/** Prize amount assigned to the tier currently being processed. */
		uint64 tierPool;
		/** Sum of integer payout floors before remainder distribution. */
		uint64 floorSum;
		/** Whole currency units still unallocated after integer division. */
		uint64 remainder;
		/** Current candidate's division remainder for deterministic tie-breaking. */
		uint64 fraction;
		/** Largest unallocated fractional remainder found in the current pass. */
		uint64 bestFraction;
		/** Currency amount assigned or transferred to the current ticket. */
		uint64 payout;
		/** Loop cursor for the bounded collection being processed. */
		uint64 i;
		/** Tier selected to receive the next rounding unit. */
		uint16 bestTier;
		/** Zero-based game, ticket, result, or accounting slot under operation. */
		uint16 slot;
		/** Whether an eligible tier was found for a rounding unit. */
		bit foundTier;
	};

	/** Validated data consumed by the process game operation. */
	struct ProcessGame_input
	{
		/** Maximum state-changing work allowed in this invocation. */
		uint64 actionBudget;
		/** Zero-based game, ticket, result, or accounting slot under operation. */
		uint16 slot;
	};
	/** Result data produced by the process game operation. */
	struct ProcessGame_output
	{
		/** Number of bounded settlement or reclamation actions consumed. */
		uint64 actionsUsed;
		/** Public outcome describing success or the reason no state change occurred. */
		EReturnCode returnCode;
	};
	/** QPI scratch state for process game; contract routines cannot declare stack locals. */
	struct ProcessGame_locals
	{
		/** Request passed to the lifecycle helper. */
		EvaluateGameLifecycle_input lifecycleInput;
		/** Response returned by the lifecycle helper. */
		EvaluateGameLifecycle_output lifecycleOutput;
		/** Request passed to the begin helper. */
		BeginSettlement_input beginInput;
		/** Response returned by the begin helper. */
		BeginSettlement_output beginOutput;
		/** Request passed to the advance helper. */
		AdvanceSettlement_input advanceInput;
		/** Response returned by the advance helper. */
		AdvanceSettlement_output advanceOutput;
		/** Request passed to the finalize helper. */
		FinalizeGame_input finalizeInput;
		/** Response returned by the finalize helper. */
		FinalizeGame_output finalizeOutput;
		/** Working copy or returned snapshot of a game record. */
		Game game;
		/** Current UTC time obtained from QPI for lifecycle decisions. */
		DateAndTime now;
	};
	/** Validated data consumed by the reclaim completed tickets operation. */
	struct ReclaimCompletedTickets_input
	{
		/** Maximum state-changing work allowed in this invocation. */
		uint64 actionBudget;
	};
	/** Result data produced by the reclaim completed tickets operation. */
	struct ReclaimCompletedTickets_output
	{
		/** Number of bounded settlement or reclamation actions consumed. */
		uint64 actionsUsed;
	};
	/** QPI scratch state for reclaim completed tickets; contract routines cannot declare stack locals. */
	struct ReclaimCompletedTickets_locals
	{
		/** Working copy or returned snapshot of a round result. */
		GameResult result;
		/** Working copy or returned snapshot of a ticket record. */
		Ticket ticket;
		/** Remaining actions available to the resumable operation. */
		uint64 budget;
		/** Ring-buffer slot selected for a round result. */
		uint64 resultIndex;
		/** Zero-based ticket slot extracted from a ticket id or reclaim cursor. */
		uint64 ticketSlot;
		/** One-based link to the next ticket or zero at the chain end. */
		uint64 nextLink;
		/** Number of result slots inspected during bounded lookup. */
		uint16 resultScans;
	};

	/** QPI scratch state for periodic lifecycle automation and ticket reclamation. */
	struct BEGIN_TICK_locals
	{
		/** Request passed to the process helper. */
		ProcessGame_input processInput;
		/** Response returned by the process helper. */
		ProcessGame_output processOutput;
		/** Request passed to the reclaim helper. */
		ReclaimCompletedTickets_input reclaimInput;
		/** Response returned by the reclaim helper. */
		ReclaimCompletedTickets_output reclaimOutput;
		/** Maximum state-changing work allowed in this invocation. */
		uint64 actionBudget;
		/** Number of game slots examined during this automation pass. */
		uint16 inspected;
		/** Zero-based game, ticket, result, or accounting slot under operation. */
		uint16 slot;
	};

	/** QPI scratch state for withdraw platform revenue; contract routines cannot declare stack locals. */
	struct WithdrawPlatformRevenue_locals
	{
		/** QPI transfer result; a negative value indicates failure. */
		sint64 transferResult;
		/** Whether any resumable transfer attempted in this operation failed. */
		bit failed;
	};

	REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
	{
		REGISTER_USER_PROCEDURE(CreateGame, 1);
		REGISTER_USER_PROCEDURE(FundGame, 3);
		REGISTER_USER_PROCEDURE(BuyTicket, 4);
		REGISTER_USER_PROCEDURE(UpdateGameEconomics, 5);
		REGISTER_USER_PROCEDURE(StopGame, 7);
		REGISTER_USER_PROCEDURE(SetPlatformConfig, 8);
		REGISTER_USER_PROCEDURE(WithdrawPlatformRevenue, 9);
		REGISTER_USER_PROCEDURE(WithdrawGameBalance, 10);
		REGISTER_USER_PROCEDURE(TransferShareManagementRights, 11);
		REGISTER_USER_PROCEDURE(BuyTickets, 15);
		REGISTER_USER_PROCEDURE(WithdrawAssetPlatformRevenue, 16);

		REGISTER_USER_FUNCTION(GetGame, 1);
		REGISTER_USER_FUNCTION(GetRoundResult, 2);
		REGISTER_USER_FUNCTION(GetTicket, 3);
		REGISTER_USER_FUNCTION(GetWinners, 4);
		REGISTER_USER_FUNCTION(GetPlatformAccounting, 5);
		REGISTER_USER_FUNCTION(ValidateDigits, 6);
		REGISTER_USER_FUNCTION(GetPlayerTickets, 7);
		REGISTER_USER_FUNCTION(PreviewGame, 8);
		REGISTER_USER_FUNCTION(GetGames, 9);
	}

	INITIALIZE()
	{
		state.mut().platformOwner =
		    ID(_R, _O, _J, _V, _A, _E, _M, _F, _B, _X, _X, _Y, _N, _G, _A, _U, _A, _U, _I, _I, _X, _L, _B, _U, _P, _D, _H, _C, _D, _P, _E, _S, _Y, _Z,
		       _O, _V, _W, _U, _Y, _E, _C, _B, _Q, _V, _Z, _R, _F, _T, _K, _A, _G, _S, _H, _T, _N, _A);
		state.mut().platformFeePercent = PLDT_PLATFORM_FEE_PERCENT;
		state.mut().maxCreatorFeePercent = PLDT_DEFAULT_MAX_CREATOR_FEE_PERCENT;
		state.mut().roundFee = PLDT_DEFAULT_ROUND_FEE;
	}

	PRE_ACQUIRE_SHARES()
	{
		output.requestedFee = 0;
		output.allowTransfer = qpi.originator() == input.owner && qpi.originator() == input.possessor;
	}

	BEGIN_TICK_WITH_LOCALS()
	{
		if (mod(qpi.tick(), PLDT_TICK_UPDATE_PERIOD) != 0)
		{
			return;
		}
		locals.inspected = 0;
		locals.actionBudget = PLDT_SETTLEMENT_ACTION_BUDGET;
		while (locals.inspected < PLDT_AUTOMATION_GAMES_PER_TICK)
		{
			locals.slot =
			    static_cast<uint16>(mod(static_cast<uint64>(state.get().automationCursor + locals.inspected), static_cast<uint64>(PLDT_MAX_GAMES)));
			if (state.get().games.get(locals.slot).status != EGameStatus::EMPTY_SLOT)
			{
				locals.processInput.slot = locals.slot;
				locals.processInput.actionBudget = locals.actionBudget;
				CALL(ProcessGame, locals.processInput, locals.processOutput);
				locals.actionBudget -= locals.processOutput.actionsUsed;
			}
			++locals.inspected;
		}
		state.mut().automationCursor =
		    static_cast<uint16>(mod(static_cast<uint64>(state.get().automationCursor + locals.inspected), static_cast<uint64>(PLDT_MAX_GAMES)));
		locals.reclaimInput.actionBudget = locals.actionBudget;
		CALL(ReclaimCompletedTickets, locals.reclaimInput, locals.reclaimOutput);
	}

	/**
	 * @brief Validates a proposed game and previews its initial funding and accounting.
	 * @param input Complete game configuration, lifecycle mode, and initial ledgers.
	 * @param output Required funding, per-ticket economics, and validation result.
	 */
	PUBLIC_FUNCTION_WITH_LOCALS(PreviewGame)
	{
		// Validate the scheduling window, bounded configuration, and custody managers without mutating state.
		output.returnCode = EReturnCode::INVALID_VALUE;
		locals.weightTotal = 0;
		locals.managedCurrencyShares = 0;
		locals.maxDrawAt = qpi.now();
		if (!locals.maxDrawAt.addDays(PLDT_MAX_SCHEDULE_DAYS))
		{
			return;
		}
		if (input.currencyMode == ECurrencyMode::ASSET)
		{
			locals.managedCurrencyShares = qpi.numberOfShares(input.currencyAsset, AssetOwnershipSelect::byManagingContract(SELF_INDEX),
			                                                  AssetPossessionSelect::byManagingContract(SELF_INDEX));
		}
		if (input.initialRunCredit < state.get().roundFee || input.initialCreatorBalance < input.creatorPrizeSeed)
		{
			output.returnCode = EReturnCode::INSUFFICIENT_FUNDS;
			return;
		}
		if (!input.startAt.isValid() || !input.drawAt.isValid() || input.startAt <= qpi.now() || input.drawAt > locals.maxDrawAt ||
		    input.drawAt <= input.startAt)
		{
			return;
		}
		if (input.ticketPrice == 0 || input.ticketPrice > PLDT_MAX_TRANSFER_AMOUNT || input.creatorPrizeSeed > PLDT_MAX_TRANSFER_AMOUNT ||
		    input.initialRunCredit > PLDT_MAX_TRANSFER_AMOUNT || input.initialCreatorBalance > PLDT_MAX_TRANSFER_AMOUNT ||
		    (input.currencyMode == ECurrencyMode::QUBIC && input.initialRunCredit > PLDT_MAX_TRANSFER_AMOUNT - input.initialCreatorBalance))
		{
			return;
		}
		if (input.ticketLimit == 0 || input.ticketLimit > PLDT_MAX_TICKETS_PER_GAME || input.playerTicketLimit == 0 ||
		    input.playerTicketLimit > input.ticketLimit)
		{
			return;
		}
		if (input.codeLength == 0 || input.codeLength > PLDT_MAX_CODE_LENGTH || input.maxDigit > PLDT_MAX_DIGIT ||
		    (!input.allowRepeatedDigits && input.maxDigit + 1 < input.codeLength))
		{
			return;
		}
		if (input.creatorFeePercent > state.get().maxCreatorFeePercent || input.creatorFeePercent > PLDT_MAX_CREATOR_FEE_PERCENT ||
		    input.bonusAssetCount > PLDT_MAX_BONUS_ASSETS)
		{
			return;
		}
		if ((input.mode != EGameMode::ONE_SHOT && input.mode != EGameMode::PERMANENT) ||
		    (input.creatorRevenueMode != ECreatorRevenueMode::PAYOUT && input.creatorRevenueMode != ECreatorRevenueMode::REINVEST) ||
		    (input.currencyMode != ECurrencyMode::QUBIC && input.currencyMode != ECurrencyMode::ASSET))
		{
			return;
		}
		if (input.bonusAssetCount > 0 &&
		    (input.bonusMultiplierBps < PLDT_BONUS_MULTIPLIER_SCALE || input.bonusMultiplierBps > PLDT_MAX_BONUS_MULTIPLIER_BPS))
		{
			return;
		}
		if (input.currencyMode == ECurrencyMode::ASSET &&
		    (input.currencyAsset.assetName == 0 || !qpi.isAssetIssued(input.currencyAsset.issuer, input.currencyAsset.assetName) ||
		     locals.managedCurrencyShares <= 0 || input.ownershipManagingContractIndex != SELF_INDEX ||
		     input.possessionManagingContractIndex != SELF_INDEX))
		{
			return;
		}

		// Verify every optional bonus asset and the complete payout matrix before calculating economics.
		for (locals.i = 0; locals.i < input.bonusAssetCount; ++locals.i)
		{
			if (input.bonusAssets.get(locals.i).assetName == 0 ||
			    !qpi.isAssetIssued(input.bonusAssets.get(locals.i).issuer, input.bonusAssets.get(locals.i).assetName))
			{
				return;
			}
		}
		for (locals.exact = 0; locals.exact <= PLDT_MAX_CODE_LENGTH; ++locals.exact)
		{
			for (locals.misplaced = 0; locals.misplaced <= PLDT_MAX_CODE_LENGTH - locals.exact; ++locals.misplaced)
			{
				locals.i = payoutMatrixIndex(locals.exact, locals.misplaced);
				locals.weightTotal = sadd(locals.weightTotal, static_cast<uint64>(input.tierWeightsBps.get(static_cast<uint16>(locals.i))));
				if (input.tierWeightsBps.get(static_cast<uint16>(locals.i)) > 0 &&
				    (locals.exact > input.codeLength || locals.misplaced > input.codeLength - locals.exact ||
				     (locals.exact + 1 == input.codeLength && locals.misplaced == 1)))
				{
					return;
				}
			}
		}
		if (locals.weightTotal != PLDT_TIER_BPS_SCALE)
		{
			return;
		}
		// Ensure gross revenue and every terminally refundable amount fit QPI's transfer bound.
		locals.economicsInput.ticketPrice = input.ticketPrice;
		locals.economicsInput.creatorFeePercent = input.creatorFeePercent;
		locals.economicsInput.platformFeePercent = state.get().platformFeePercent;
		CALL(CalculateTicketEconomics, locals.economicsInput, locals.economicsOutput);
		if (input.currencyMode == ECurrencyMode::ASSET && input.currencyAsset.issuer == NULL_ID && locals.economicsOutput.burn > 0)
		{
			// QPI intentionally forbids burning issuer-zero contract shares.
			return;
		}
		locals.refundablePerTicket = sadd(locals.economicsOutput.creatorFee, locals.economicsOutput.prizeContribution);
		if (input.ticketPrice > div(PLDT_MAX_TRANSFER_AMOUNT, static_cast<uint64>(input.ticketLimit)) ||
		    locals.refundablePerTicket > div(PLDT_MAX_TRANSFER_AMOUNT - input.creatorPrizeSeed, static_cast<uint64>(input.ticketLimit)))
		{
			return;
		}
		output.platformFee = locals.economicsOutput.platformFee;
		output.roundFee = state.get().roundFee;
		if (input.currencyMode == ECurrencyMode::QUBIC)
		{
			output.initialQubicRequired = sadd(input.initialRunCredit, input.initialCreatorBalance);
		}
		else
		{
			output.initialQubicRequired = input.initialRunCredit;
			output.initialCreatorAssetRequired = input.initialCreatorBalance;
		}
		output.creatorFee = locals.economicsOutput.creatorFee;
		output.burn = locals.economicsOutput.burn;
		output.prizeContribution = locals.economicsOutput.prizeContribution;
		output.returnCode = EReturnCode::SUCCESS;
	}

	/**
	 * @brief Atomically creates, funds, and charges a one-shot or permanent game.
	 * @param input Game rules, lifecycle mode, UTC window, economics, currency, and initial ledgers.
	 * @param output Generation-aware game id, slot, and result code.
	 * @note The first round fee is non-refundable; asset creator funding is pulled from the creator.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(CreateGame)
	{
		// Complete all configuration, payment, and accrual-capacity checks before taking asset custody.
		output.gameId = 0;
		output.slot = 0;
		locals.previewInput = input;
		CALL(PreviewGame, locals.previewInput, locals.previewOutput);
		locals.maxDrawAt = qpi.now();
		if (locals.previewOutput.returnCode != EReturnCode::SUCCESS)
		{
			locals.refundInput.returnCode = locals.previewOutput.returnCode;
			CALL(RefundInvocationReward, locals.refundInput, locals.refundOutput);
			output.returnCode = locals.refundOutput.returnCode;
			return;
		}
		if (input.startAt <= qpi.now() || !locals.maxDrawAt.addDays(PLDT_MAX_SCHEDULE_DAYS) || input.drawAt > locals.maxDrawAt)
		{
			locals.refundInput.returnCode = EReturnCode::INVALID_VALUE;
			CALL(RefundInvocationReward, locals.refundInput, locals.refundOutput);
			output.returnCode = locals.refundOutput.returnCode;
			return;
		}
		locals.expectedReward = locals.previewOutput.initialQubicRequired;
		if (qpi.invocationReward() != locals.expectedReward)
		{
			locals.refundInput.returnCode = EReturnCode::TICKET_INVALID_PRICE;
			CALL(RefundInvocationReward, locals.refundInput, locals.refundOutput);
			output.returnCode = locals.refundOutput.returnCode;
			return;
		}
		locals.developer1Fee = mulDiv(state.get().roundFee, PLDT_PLATFORM_DEV1_SHARE_PERCENT, 100ULL);
		locals.developer2Fee = mulDiv(state.get().roundFee, PLDT_PLATFORM_DEV2_SHARE_PERCENT, 100ULL);
		locals.dividendFee = state.get().roundFee - locals.developer1Fee - locals.developer2Fee;
		if (state.get().developer1Accrued > PLDT_MAX_TRANSFER_AMOUNT - locals.developer1Fee ||
		    state.get().developer2Accrued > PLDT_MAX_TRANSFER_AMOUNT - locals.developer2Fee ||
		    state.get().dividendAccrued > PLDT_MAX_TRANSFER_AMOUNT - locals.dividendFee)
		{
			locals.refundInput.returnCode = EReturnCode::STORAGE_FULL;
			CALL(RefundInvocationReward, locals.refundInput, locals.refundOutput);
			output.returnCode = locals.refundOutput.returnCode;
			return;
		}
		// Find a reusable slot while enforcing the creator-wide active-game limit in the same bounded scan.
		locals.found = false;
		locals.creatorActiveGames = 0;
		for (locals.i = 0; locals.i < PLDT_MAX_GAMES; ++locals.i)
		{
			locals.game = state.get().games.get(static_cast<uint16>(locals.i));
			if (locals.game.status != EGameStatus::EMPTY_SLOT && locals.game.owner == qpi.invocator())
			{
				++locals.creatorActiveGames;
			}
			locals.candidateSlot = static_cast<uint16>(mod(state.get().allocationCursor + locals.i, static_cast<uint64>(PLDT_MAX_GAMES)));
			if (!locals.found && state.get().games.get(locals.candidateSlot).status == EGameStatus::EMPTY_SLOT)
			{
				locals.slot = locals.candidateSlot;
				locals.found = true;
			}
		}
		if (!locals.found || locals.creatorActiveGames >= PLDT_MAX_ACTIVE_GAMES_PER_CREATOR)
		{
			locals.refundInput.returnCode = EReturnCode::STORAGE_FULL;
			CALL(RefundInvocationReward, locals.refundInput, locals.refundOutput);
			output.returnCode = locals.refundOutput.returnCode;
			return;
		}
		// Resolve the asset accounting bucket and take creator funding only after every Qubic-side check passes.
		if (input.currencyMode == ECurrencyMode::ASSET)
		{
			locals.accountingFound = false;
			for (locals.i = 0; locals.i < state.get().assetAccounting.capacity(); ++locals.i)
			{
				locals.assetAccounting = state.get().assetAccounting.get(locals.i);
				if (locals.assetAccounting.isActive && locals.assetAccounting.asset.assetName == input.currencyAsset.assetName &&
				    locals.assetAccounting.asset.issuer == input.currencyAsset.issuer &&
				    locals.assetAccounting.ownershipManagingContractIndex == input.ownershipManagingContractIndex &&
				    locals.assetAccounting.possessionManagingContractIndex == input.possessionManagingContractIndex)
				{
					locals.accountingSlot = static_cast<uint16>(locals.i);
					locals.accountingFound = true;
					break;
				}
			}
			if (!locals.accountingFound)
			{
				for (locals.i = 0; locals.i < state.get().assetAccounting.capacity(); ++locals.i)
				{
					locals.assetAccounting = state.get().assetAccounting.get(locals.i);
					if (!locals.assetAccounting.isActive)
					{
						locals.accountingSlot = static_cast<uint16>(locals.i);
						locals.assetAccounting.asset = input.currencyAsset;
						locals.assetAccounting.ownershipManagingContractIndex = input.ownershipManagingContractIndex;
						locals.assetAccounting.possessionManagingContractIndex = input.possessionManagingContractIndex;
						locals.assetAccounting.isActive = true;
						locals.accountingFound = true;
						break;
					}
				}
			}
			if (!locals.accountingFound)
			{
				locals.refundInput.returnCode = EReturnCode::STORAGE_FULL;
				CALL(RefundInvocationReward, locals.refundInput, locals.refundOutput);
				output.returnCode = locals.refundOutput.returnCode;
				return;
			}
			if (input.initialCreatorBalance > 0)
			{
				locals.possessedShares =
				    qpi.numberOfPossessedShares(input.currencyAsset.assetName, input.currencyAsset.issuer, qpi.invocator(), qpi.invocator(),
				                                input.ownershipManagingContractIndex, input.possessionManagingContractIndex);
				if (locals.possessedShares < static_cast<sint64>(input.initialCreatorBalance))
				{
					locals.refundInput.returnCode = EReturnCode::INSUFFICIENT_FUNDS;
					CALL(RefundInvocationReward, locals.refundInput, locals.refundOutput);
					output.returnCode = locals.refundOutput.returnCode;
					return;
				}
				locals.transferResult =
				    qpi.transferShareOwnershipAndPossession(input.currencyAsset.assetName, input.currencyAsset.issuer, qpi.invocator(),
				                                            qpi.invocator(), static_cast<sint64>(input.initialCreatorBalance), SELF);
				if (locals.transferResult < 0)
				{
					locals.refundInput.returnCode = EReturnCode::TRANSFER_FAILED;
					CALL(RefundInvocationReward, locals.refundInput, locals.refundOutput);
					output.returnCode = locals.refundOutput.returnCode;
					return;
				}
			}
		}
		// Commit a fresh slot generation and all accounting together so stale ids cannot alias the new game.
		locals.generation = sadd(state.get().generations.get(locals.slot), 1ULL);
		if (locals.generation == 0)
		{
			locals.generation = 1;
		}
		state.mut().generations.set(locals.slot, locals.generation);
		setMemory(locals.game, 0);
		locals.game.tierWeightsBps = input.tierWeightsBps;
		locals.game.bonusAssets = input.bonusAssets;
		locals.game.name = input.name;
		locals.game.currencyAsset = input.currencyAsset;
		locals.game.owner = qpi.invocator();
		locals.game.startAt = input.startAt;
		locals.game.drawAt = input.drawAt;
		locals.game.gameId = (locals.generation << 10) | locals.slot;
		locals.game.ticketPrice = input.ticketPrice;
		locals.game.creatorPrizeSeed = input.creatorPrizeSeed;
		locals.game.prizePool = input.creatorPrizeSeed;
		locals.game.roundFeeSnapshot = state.get().roundFee;
		locals.game.runCredit = input.initialRunCredit - state.get().roundFee;
		locals.game.creatorBalance = input.initialCreatorBalance - input.creatorPrizeSeed;
		locals.game.roundDurationMicroseconds = input.startAt.durationMicrosec(input.drawAt);
		locals.game.bonusMultiplierBps = input.bonusMultiplierBps;
		locals.game.ticketLimit = input.ticketLimit;
		locals.game.playerTicketLimit = input.playerTicketLimit;
		locals.game.bonusAssetCount = input.bonusAssetCount;
		locals.game.ownershipManagingContractIndex = input.ownershipManagingContractIndex;
		locals.game.possessionManagingContractIndex = input.possessionManagingContractIndex;
		locals.game.bonusOwnershipManagingContractIndex = input.bonusOwnershipManagingContractIndex;
		locals.game.bonusPossessionManagingContractIndex = input.bonusPossessionManagingContractIndex;
		locals.game.assetAccountingLink = input.currencyMode == ECurrencyMode::ASSET ? locals.accountingSlot + 1 : 0;
		locals.game.codeLength = input.codeLength;
		locals.game.maxDigit = input.maxDigit;
		locals.game.creatorFeePercent = input.creatorFeePercent;
		locals.game.currencyMode = input.currencyMode;
		locals.game.mode = input.mode;
		locals.game.creatorRevenueMode = input.creatorRevenueMode;
		locals.game.roundNumber = 1;
		locals.game.allowRepeatedDigits = input.allowRepeatedDigits;
		locals.game.status = EGameStatus::SCHEDULED;
		if (input.currencyMode == ECurrencyMode::ASSET)
		{
			++locals.assetAccounting.activeGameCount;
			state.mut().assetAccounting.set(locals.accountingSlot, locals.assetAccounting);
		}
		state.mut().games.set(locals.slot, locals.game);
		state.mut().developer1Accrued = sadd(state.get().developer1Accrued, locals.developer1Fee);
		state.mut().developer2Accrued = sadd(state.get().developer2Accrued, locals.developer2Fee);
		state.mut().dividendAccrued = sadd(state.get().dividendAccrued, state.get().roundFee - locals.developer1Fee - locals.developer2Fee);
		state.mut().activeGameCount = state.get().activeGameCount + 1;
		state.mut().allocationCursor = static_cast<uint16>(mod(static_cast<uint64>(locals.slot + 1), static_cast<uint64>(PLDT_MAX_GAMES)));
		output.gameId = locals.game.gameId;
		output.slot = locals.slot;
		output.returnCode = EReturnCode::SUCCESS;
	}

	/**
	 * @brief Adds Qubic run credit and/or game-currency creator balance.
	 * @param input Game id and ledger top-ups.
	 * @param output Result code; failed operations preserve both ledgers.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(FundGame)
	{
		// Authorize and capacity-check both ledgers before taking any asset shares.
		locals.slot = gameSlot(input.gameId);
		if (!isGameIdValid(state, input.gameId))
		{
			locals.refundInput.returnCode = EReturnCode::INVALID_GAME;
			CALL(RefundInvocationReward, locals.refundInput, locals.refundOutput);
			output.returnCode = locals.refundOutput.returnCode;
			return;
		}
		locals.game = state.get().games.get(locals.slot);
		if (locals.game.owner != qpi.invocator())
		{
			locals.refundInput.returnCode = EReturnCode::ACCESS_DENIED;
			CALL(RefundInvocationReward, locals.refundInput, locals.refundOutput);
			output.returnCode = locals.refundOutput.returnCode;
			return;
		}
		if (locals.game.status == EGameStatus::FINALIZING)
		{
			locals.refundInput.returnCode = EReturnCode::INVALID_STATE;
			CALL(RefundInvocationReward, locals.refundInput, locals.refundOutput);
			output.returnCode = locals.refundOutput.returnCode;
			return;
		}
		locals.expectedReward = input.runCreditTopUp;
		if (locals.game.currencyMode == ECurrencyMode::QUBIC)
		{
			if (input.runCreditTopUp > PLDT_MAX_TRANSFER_AMOUNT - input.creatorBalanceTopUp)
			{
				locals.refundInput.returnCode = EReturnCode::INVALID_VALUE;
				CALL(RefundInvocationReward, locals.refundInput, locals.refundOutput);
				output.returnCode = locals.refundOutput.returnCode;
				return;
			}
			locals.expectedReward = sadd(locals.expectedReward, input.creatorBalanceTopUp);
		}
		if (qpi.invocationReward() != locals.expectedReward)
		{
			locals.refundInput.returnCode = EReturnCode::INVALID_VALUE;
			CALL(RefundInvocationReward, locals.refundInput, locals.refundOutput);
			output.returnCode = locals.refundOutput.returnCode;
			return;
		}
		if (input.runCreditTopUp > PLDT_MAX_TRANSFER_AMOUNT - locals.game.runCredit ||
		    locals.game.creatorBalance > PLDT_MAX_TRANSFER_AMOUNT - locals.game.prizePool ||
		    input.creatorBalanceTopUp > PLDT_MAX_TRANSFER_AMOUNT - locals.game.prizePool - locals.game.creatorBalance)
		{
			locals.refundInput.returnCode = EReturnCode::INVALID_VALUE;
			CALL(RefundInvocationReward, locals.refundInput, locals.refundOutput);
			output.returnCode = locals.refundOutput.returnCode;
			return;
		}
		// Asset creator balance is pulled only after the Qubic invocation reward and all invariants are valid.
		if (locals.game.currencyMode == ECurrencyMode::ASSET && input.creatorBalanceTopUp > 0)
		{
			locals.possessedShares =
			    qpi.numberOfPossessedShares(locals.game.currencyAsset.assetName, locals.game.currencyAsset.issuer, qpi.invocator(), qpi.invocator(),
			                                locals.game.ownershipManagingContractIndex, locals.game.possessionManagingContractIndex);
			if (locals.possessedShares < static_cast<sint64>(input.creatorBalanceTopUp))
			{
				locals.refundInput.returnCode = EReturnCode::INSUFFICIENT_FUNDS;
				CALL(RefundInvocationReward, locals.refundInput, locals.refundOutput);
				output.returnCode = locals.refundOutput.returnCode;
				return;
			}
			locals.transferResult =
			    qpi.transferShareOwnershipAndPossession(locals.game.currencyAsset.assetName, locals.game.currencyAsset.issuer, qpi.invocator(),
			                                            qpi.invocator(), static_cast<sint64>(input.creatorBalanceTopUp), SELF);
			if (locals.transferResult < 0)
			{
				locals.refundInput.returnCode = EReturnCode::TRANSFER_FAILED;
				CALL(RefundInvocationReward, locals.refundInput, locals.refundOutput);
				output.returnCode = locals.refundOutput.returnCode;
				return;
			}
		}
		locals.game.runCredit = sadd(locals.game.runCredit, input.runCreditTopUp);
		locals.game.creatorBalance = sadd(locals.game.creatorBalance, input.creatorBalanceTopUp);
		state.mut().games.set(locals.slot, locals.game);
		output.returnCode = EReturnCode::SUCCESS;
	}

	/**
	 * @brief Withdraws unreserved game ledgers to their owner.
	 * @param input Requested amounts; the current prize pool is not addressable.
	 * @param output Successfully transferred amounts and result code.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(WithdrawGameBalance)
	{
		locals.slot = gameSlot(input.gameId);
		if (!isGameIdValid(state, input.gameId))
		{
			output.returnCode = EReturnCode::INVALID_GAME;
			return;
		}
		locals.game = state.get().games.get(locals.slot);
		if (locals.game.owner != qpi.invocator())
		{
			output.returnCode = EReturnCode::ACCESS_DENIED;
			return;
		}
		if (locals.game.status == EGameStatus::FINALIZING)
		{
			output.returnCode = EReturnCode::INVALID_STATE;
			return;
		}
		if (input.runCreditAmount > locals.game.runCredit || input.creatorBalanceAmount > locals.game.creatorBalance)
		{
			output.returnCode = EReturnCode::INSUFFICIENT_FUNDS;
			return;
		}
		locals.failed = false;
		if (locals.game.currencyMode == ECurrencyMode::QUBIC)
		{
			if (input.runCreditAmount > 0)
			{
				locals.transferResult = qpi.transfer(locals.game.owner, static_cast<sint64>(input.runCreditAmount));
				if (locals.transferResult >= 0)
				{
					locals.game.runCredit -= input.runCreditAmount;
					output.runCreditPaid = input.runCreditAmount;
				}
				else
				{
					locals.failed = true;
				}
			}
			if (input.creatorBalanceAmount > 0)
			{
				locals.transferResult = qpi.transfer(locals.game.owner, static_cast<sint64>(input.creatorBalanceAmount));
				if (locals.transferResult >= 0)
				{
					locals.game.creatorBalance -= input.creatorBalanceAmount;
					output.creatorBalancePaid = input.creatorBalanceAmount;
				}
				else
				{
					locals.failed = true;
				}
			}
		}
		else
		{
			if (input.runCreditAmount > 0)
			{
				locals.transferResult = qpi.transfer(locals.game.owner, static_cast<sint64>(input.runCreditAmount));
				if (locals.transferResult >= 0)
				{
					locals.game.runCredit -= input.runCreditAmount;
					output.runCreditPaid = input.runCreditAmount;
				}
				else
				{
					locals.failed = true;
				}
			}
			if (input.creatorBalanceAmount > 0)
			{
				locals.transferResult =
				    qpi.transferShareOwnershipAndPossession(locals.game.currencyAsset.assetName, locals.game.currencyAsset.issuer, SELF, SELF,
				                                            static_cast<sint64>(input.creatorBalanceAmount), locals.game.owner);
				if (locals.transferResult >= 0)
				{
					locals.game.creatorBalance -= input.creatorBalanceAmount;
					output.creatorBalancePaid = input.creatorBalanceAmount;
				}
				else
				{
					locals.failed = true;
				}
			}
		}
		state.mut().games.set(locals.slot, locals.game);
		output.returnCode = locals.failed ? EReturnCode::TRANSFER_FAILED : EReturnCode::SUCCESS;
	}

	/**
	 * @brief Queues validated economics for the next round of a permanent game.
	 * @param input Complete replace-on-write next-round economics.
	 * @param output Result code; the current round is never modified.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(UpdateGameEconomics)
	{
		locals.slot = gameSlot(input.gameId);
		if (!isGameIdValid(state, input.gameId))
		{
			output.returnCode = EReturnCode::INVALID_GAME;
			return;
		}
		locals.game = state.get().games.get(locals.slot);
		if (locals.game.mode != EGameMode::PERMANENT)
		{
			output.returnCode = EReturnCode::INVALID_STATE;
			return;
		}
		if (locals.game.owner != qpi.invocator())
		{
			output.returnCode = EReturnCode::ACCESS_DENIED;
			return;
		}
		if (locals.game.status == EGameStatus::FINALIZING)
		{
			output.returnCode = EReturnCode::INVALID_STATE;
			return;
		}
		if (input.ticketPrice == 0 || input.ticketPrice > PLDT_MAX_TRANSFER_AMOUNT || input.creatorPrizeSeed > PLDT_MAX_TRANSFER_AMOUNT)
		{
			output.returnCode = EReturnCode::INVALID_VALUE;
			return;
		}
		if (input.ticketLimit == 0 || input.ticketLimit > PLDT_MAX_TICKETS_PER_GAME || input.playerTicketLimit == 0 ||
		    input.playerTicketLimit > input.ticketLimit)
		{
			output.returnCode = EReturnCode::INVALID_VALUE;
			return;
		}
		if (input.creatorFeePercent > state.get().maxCreatorFeePercent)
		{
			output.returnCode = EReturnCode::INVALID_VALUE;
			return;
		}
		if (input.creatorRevenueMode != ECreatorRevenueMode::PAYOUT && input.creatorRevenueMode != ECreatorRevenueMode::REINVEST)
		{
			output.returnCode = EReturnCode::INVALID_VALUE;
			return;
		}
		locals.economicsInput.ticketPrice = input.ticketPrice;
		locals.economicsInput.creatorFeePercent = input.creatorFeePercent;
		locals.economicsInput.platformFeePercent = state.get().platformFeePercent;
		CALL(CalculateTicketEconomics, locals.economicsInput, locals.economicsOutput);
		if (locals.game.currencyMode == ECurrencyMode::ASSET && locals.game.currencyAsset.issuer == NULL_ID && locals.economicsOutput.burn > 0)
		{
			output.returnCode = EReturnCode::INVALID_VALUE;
			return;
		}
		locals.refundablePerTicket = sadd(locals.economicsOutput.creatorFee, locals.economicsOutput.prizeContribution);
		if (input.ticketPrice > div(PLDT_MAX_TRANSFER_AMOUNT, static_cast<uint64>(input.ticketLimit)) ||
		    locals.refundablePerTicket > div(PLDT_MAX_TRANSFER_AMOUNT - input.creatorPrizeSeed, static_cast<uint64>(input.ticketLimit)))
		{
			output.returnCode = EReturnCode::INVALID_VALUE;
			return;
		}
		setMemory(locals.pending, 0);
		locals.pending.ticketPrice = input.ticketPrice;
		locals.pending.creatorPrizeSeed = input.creatorPrizeSeed;
		locals.pending.ticketLimit = input.ticketLimit;
		locals.pending.playerTicketLimit = input.playerTicketLimit;
		locals.pending.creatorFeePercent = input.creatorFeePercent;
		locals.pending.creatorRevenueMode = input.creatorRevenueMode;
		locals.pending.isSet = true;
		locals.game.pendingEconomics = locals.pending;
		state.mut().games.set(locals.slot, locals.game);
		output.returnCode = EReturnCode::SUCCESS;
	}

	/**
	 * @brief Cancels either mode before first start or requests terminal closure when applicable.
	 * @param input Game id.
	 * @param output Result code; an active one-shot round is unchanged and an active permanent round is not shortened.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(StopGame)
	{
		locals.slot = gameSlot(input.gameId);
		if (!isGameIdValid(state, input.gameId))
		{
			output.returnCode = EReturnCode::INVALID_GAME;
			return;
		}
		locals.game = state.get().games.get(locals.slot);
		if (locals.game.owner != qpi.invocator())
		{
			output.returnCode = EReturnCode::ACCESS_DENIED;
			return;
		}
		if (locals.game.roundNumber == 1 && locals.game.status == EGameStatus::SCHEDULED && qpi.now() < locals.game.startAt)
		{
			locals.finalizeInput.slot = locals.slot;
			locals.finalizeInput.reason = EGameTerminalReason::OWNER_CANCELLED;
			CALL(FinalizeGame, locals.finalizeInput, locals.finalizeOutput);
			output.returnCode = locals.finalizeOutput.returnCode;
			return;
		}
		if (locals.game.mode == EGameMode::ONE_SHOT)
		{
			output.returnCode = EReturnCode::SUCCESS;
			return;
		}
		if (locals.game.status == EGameStatus::FINALIZING && locals.game.finalizingStopReason == EGameStopReason::NONE)
		{
			locals.game.finalizingStopReason = EGameStopReason::OWNER_REQUESTED;
			locals.game.pendingRunCreditPayout = locals.game.runCredit;
			locals.game.runCredit = 0;
			locals.game.pendingCreatorBalancePayout = locals.game.creatorBalance;
			locals.game.creatorBalance = 0;
			locals.game.stopRequested = true;
			state.mut().games.set(locals.slot, locals.game);
			output.returnCode = EReturnCode::SUCCESS;
			return;
		}
		locals.game.stopRequested = true;
		state.mut().games.set(locals.slot, locals.game);
		output.returnCode = EReturnCode::SUCCESS;
	}

	/**
	 * @brief Buys one ticket during the effective UTC sales window.
	 * @param input Game id and submitted digits.
	 * @param output Generation-aware ticket id, compatibility slot, contribution, and result code.
	 * @note Invalid Qubic purchases refund the invocation reward.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(BuyTicket)
	{
		// Validate lifecycle, storage, digits, player limit, and every bounded ledger before custody.
		output.ticketIndex = 0;
		output.prizeContribution = 0;
		if (!isGameIdValid(state, input.gameId))
		{
			locals.refundInput.returnCode = EReturnCode::INVALID_GAME;
			CALL(RefundInvocationReward, locals.refundInput, locals.refundOutput);
			output.returnCode = locals.refundOutput.returnCode;
			return;
		}
		locals.slot = gameSlot(input.gameId);
		locals.game = state.get().games.get(locals.slot);
		locals.now = qpi.now();
		locals.lifecycleInput.game = locals.game;
		locals.lifecycleInput.now = locals.now;
		CALL(EvaluateGameLifecycle, locals.lifecycleInput, locals.lifecycleOutput);
		if (locals.lifecycleOutput.purchaseReturnCode != EReturnCode::SUCCESS)
		{
			locals.refundInput.returnCode = locals.lifecycleOutput.purchaseReturnCode;
			CALL(RefundInvocationReward, locals.refundInput, locals.refundOutput);
			output.returnCode = locals.refundOutput.returnCode;
			return;
		}
		locals.game.status = locals.lifecycleOutput.effectiveStatus;
		if (locals.game.ticketCount >= locals.game.ticketLimit ||
		    (state.get().freeTicketCount == 0 &&
		     (state.get().nextUnusedTicketSlot != 0 ? state.get().nextUnusedTicketSlot : state.get().ticketCount) >= state.get().tickets.capacity()))
		{
			locals.refundInput.returnCode = EReturnCode::TICKET_SOLD_OUT;
			CALL(RefundInvocationReward, locals.refundInput, locals.refundOutput);
			output.returnCode = locals.refundOutput.returnCode;
			return;
		}
		locals.validateInput.digits = input.digits;
		locals.validateInput.codeLength = locals.game.codeLength;
		locals.validateInput.maxDigit = locals.game.maxDigit;
		locals.validateInput.allowRepeatedDigits = locals.game.allowRepeatedDigits;
		CALL(ValidateDigits, locals.validateInput, locals.validateOutput);
		if (locals.validateOutput.returnCode != EReturnCode::SUCCESS)
		{
			locals.refundInput.returnCode = locals.validateOutput.returnCode;
			CALL(RefundInvocationReward, locals.refundInput, locals.refundOutput);
			output.returnCode = locals.refundOutput.returnCode;
			return;
		}
		locals.link = locals.game.firstTicketLink;
		locals.playerTickets = 0;
		while (locals.link != 0)
		{
			locals.ticket = state.get().tickets.get(locals.link - 1);
			if (locals.ticket.player == qpi.invocator())
			{
				++locals.playerTickets;
			}
			locals.link = locals.ticket.nextLink;
		}
		if (locals.playerTickets >= locals.game.playerTicketLimit)
		{
			locals.refundInput.returnCode = EReturnCode::PLAYER_TICKET_LIMIT;
			CALL(RefundInvocationReward, locals.refundInput, locals.refundOutput);
			output.returnCode = locals.refundOutput.returnCode;
			return;
		}
		locals.bonusInput.game = locals.game;
		locals.bonusInput.player = qpi.invocator();
		CALL(EvaluateBonusQualification, locals.bonusInput, locals.bonusOutput);
		locals.economicsInput.ticketPrice = locals.game.ticketPrice;
		locals.economicsInput.creatorFeePercent = locals.game.creatorFeePercent;
		locals.economicsInput.platformFeePercent = state.get().platformFeePercent;
		CALL(CalculateTicketEconomics, locals.economicsInput, locals.economicsOutput);
		if (locals.game.totalRevenue > PLDT_MAX_TRANSFER_AMOUNT - locals.game.ticketPrice ||
		    locals.game.creatorBalance > PLDT_MAX_TRANSFER_AMOUNT - locals.game.prizePool ||
		    locals.economicsOutput.prizeContribution > PLDT_MAX_TRANSFER_AMOUNT - locals.game.prizePool - locals.game.creatorBalance)
		{
			locals.refundInput.returnCode = EReturnCode::STORAGE_FULL;
			CALL(RefundInvocationReward, locals.refundInput, locals.refundOutput);
			output.returnCode = locals.refundOutput.returnCode;
			return;
		}
		// Take exactly one currency payment only after the purchase is known to fit atomically.
		if (locals.game.currencyMode == ECurrencyMode::QUBIC)
		{
			if (state.get().developer1Accrued > PLDT_MAX_TRANSFER_AMOUNT - locals.economicsOutput.developer1Fee ||
			    state.get().developer2Accrued > PLDT_MAX_TRANSFER_AMOUNT - locals.economicsOutput.developer2Fee ||
			    state.get().dividendAccrued > PLDT_MAX_TRANSFER_AMOUNT - locals.economicsOutput.dividendFee)
			{
				locals.refundInput.returnCode = EReturnCode::STORAGE_FULL;
				CALL(RefundInvocationReward, locals.refundInput, locals.refundOutput);
				output.returnCode = locals.refundOutput.returnCode;
				return;
			}
		}
		else
		{
			locals.assetAccounting = state.get().assetAccounting.get(locals.game.assetAccountingLink - 1);
			if (locals.assetAccounting.developer1Accrued > PLDT_MAX_TRANSFER_AMOUNT - locals.economicsOutput.developer1Fee ||
			    locals.assetAccounting.developer2Accrued > PLDT_MAX_TRANSFER_AMOUNT - locals.economicsOutput.developer2Fee ||
			    locals.assetAccounting.dividendAccrued > PLDT_MAX_TRANSFER_AMOUNT - locals.economicsOutput.dividendFee)
			{
				output.returnCode = EReturnCode::STORAGE_FULL;
				return;
			}
		}
		if (locals.game.currencyMode == ECurrencyMode::QUBIC)
		{
			if (qpi.invocationReward() != locals.game.ticketPrice)
			{
				locals.refundInput.returnCode = EReturnCode::TICKET_INVALID_PRICE;
				CALL(RefundInvocationReward, locals.refundInput, locals.refundOutput);
				output.returnCode = locals.refundOutput.returnCode;
				return;
			}
		}
		else
		{
			if (qpi.invocationReward() != 0)
			{
				locals.refundInput.returnCode = EReturnCode::TICKET_INVALID_PRICE;
				CALL(RefundInvocationReward, locals.refundInput, locals.refundOutput);
				output.returnCode = locals.refundOutput.returnCode;
				return;
			}
			locals.possessedShares =
			    qpi.numberOfPossessedShares(locals.game.currencyAsset.assetName, locals.game.currencyAsset.issuer, qpi.invocator(), qpi.invocator(),
			                                locals.game.ownershipManagingContractIndex, locals.game.possessionManagingContractIndex);
			if (locals.possessedShares < static_cast<sint64>(locals.game.ticketPrice))
			{
				output.returnCode = EReturnCode::INSUFFICIENT_FUNDS;
				return;
			}
			locals.transferResult =
			    qpi.transferShareOwnershipAndPossession(locals.game.currencyAsset.assetName, locals.game.currencyAsset.issuer, qpi.invocator(),
			                                            qpi.invocator(), static_cast<sint64>(locals.game.ticketPrice), SELF);
			if (locals.transferResult < 0)
			{
				output.returnCode = EReturnCode::TRANSFER_FAILED;
				return;
			}
		}
		// Burn first, then commit the ticket and accounting; a failed burn refunds the collected payment.
		locals.burnInput.game = locals.game;
		locals.burnInput.ticketCount = 1;
		CALL(BurnCollectedTicketPayment, locals.burnInput, locals.burnOutput);
		if (locals.burnOutput.returnCode != EReturnCode::SUCCESS)
		{
			locals.transferInput.game = locals.game;
			locals.transferInput.destination = qpi.invocator();
			locals.transferInput.amount = locals.game.ticketPrice;
			CALL(TransferGameCurrency, locals.transferInput, locals.transferOutput);
			output.returnCode = EReturnCode::TRANSFER_FAILED;
			return;
		}
		state.mut().games.set(locals.slot, locals.game);
		locals.applyInput.gameId = input.gameId;
		locals.applyInput.digits = input.digits;
		locals.applyInput.bonusQualified = locals.bonusOutput.qualified;
		CALL(ApplyAcceptedTicket, locals.applyInput, locals.applyOutput);
		output.ticketIndex = locals.applyOutput.ticketIndex;
		output.ticketId = locals.applyOutput.ticketId;
		output.prizeContribution = locals.applyOutput.prizeContribution;
		output.returnCode = locals.applyOutput.returnCode;
	}

	/**
	 * @brief Reads an active game with its effective time-based sales status.
	 * @param input Generation-aware game id.
	 * @param output Game snapshot and result code.
	 */
	PUBLIC_FUNCTION_WITH_LOCALS(GetGame)
	{
		if (!isGameIdValid(state, input.gameId))
		{
			output.returnCode = EReturnCode::INVALID_GAME;
			return;
		}
		output.game = state.get().games.get(gameSlot(input.gameId));
		locals.lifecycleInput.game = output.game;
		locals.lifecycleInput.now = qpi.now();
		CALL(EvaluateGameLifecycle, locals.lifecycleInput, locals.lifecycleOutput);
		output.game.status = locals.lifecycleOutput.effectiveStatus;
		output.returnCode = EReturnCode::SUCCESS;
	}

	/**
	 * @brief Reads a retained terminal result.
	 * @param input Generation-aware game id.
	 * @param output Result snapshot or `INVALID_GAME` after ring eviction.
	 */
	PUBLIC_FUNCTION_WITH_LOCALS(GetRoundResult)
	{
		locals.gameId = input.roundKey.gameId;
		locals.roundNumber = input.roundKey.roundNumber;
		if (locals.gameId == 0 || locals.roundNumber == 0)
		{
			output.returnCode = EReturnCode::INVALID_ROUND;
			return;
		}
		locals.foundGame = false;
		locals.available = state.get().resultCounter < PLDT_RESULT_HISTORY_SIZE ? state.get().resultCounter : PLDT_RESULT_HISTORY_SIZE;
		for (locals.i = 0; locals.i < locals.available; ++locals.i)
		{
			locals.counter = state.get().resultCounter - 1 - locals.i;
			locals.result = state.get().results.get(static_cast<uint16>(mod(locals.counter, static_cast<uint64>(PLDT_RESULT_STORAGE_SIZE))));
			if (locals.result.gameId == locals.gameId)
			{
				locals.foundGame = true;
				if (locals.result.roundNumber != locals.roundNumber)
				{
					continue;
				}
				output.roundResult = locals.result;
				output.gameResult = locals.result;
				output.returnCode = EReturnCode::SUCCESS;
				return;
			}
		}
		output.returnCode = locals.foundGame ? EReturnCode::INVALID_ROUND : EReturnCode::INVALID_GAME;
	}

	/**
	 * @brief Reads one globally indexed ticket.
	 * @param input Generation-aware ticket id.
	 * @param output Ticket snapshot and result code.
	 */
	PUBLIC_FUNCTION(GetTicket)
	{
		if (input.ticketId == 0 || (input.ticketId & PLDT_TICKET_SLOT_MASK) >= state.get().tickets.capacity() ||
		    state.get().tickets.get(input.ticketId & PLDT_TICKET_SLOT_MASK).ticketId != input.ticketId)
		{
			output.returnCode = EReturnCode::INVALID_TICKET;
			return;
		}
		output.ticket = state.get().tickets.get(input.ticketId & PLDT_TICKET_SLOT_MASK);
		output.returnCode = EReturnCode::SUCCESS;
	}

	/**
	 * @brief Validates digit bounds and the repeated-digit policy.
	 * @param input Digits and code policy.
	 * @param output `SUCCESS` or `INVALID_DIGITS`.
	 */
	PUBLIC_FUNCTION_WITH_LOCALS(ValidateDigits)
	{
		setMemory(locals.seen, 0);
		if (input.codeLength == 0 || input.codeLength > PLDT_MAX_CODE_LENGTH || input.maxDigit > PLDT_MAX_DIGIT ||
		    (!input.allowRepeatedDigits && input.maxDigit + 1 < input.codeLength))
		{
			output.returnCode = EReturnCode::INVALID_DIGITS;
			return;
		}
		for (locals.i = 0; locals.i < input.codeLength; ++locals.i)
		{
			locals.digit = input.digits.get(locals.i);
			if (locals.digit > input.maxDigit || (!input.allowRepeatedDigits && locals.seen.get(locals.digit) != 0))
			{
				output.returnCode = EReturnCode::INVALID_DIGITS;
				return;
			}
			locals.seen.set(locals.digit, 1);
		}
		output.returnCode = EReturnCode::SUCCESS;
	}

	/**
	 * @brief Pages through ticket ids for one player and round.
	 * @param input Player, game id, offset, and requested limit.
	 * @param output Matching indexes, counts, and result code.
	 */
	PUBLIC_FUNCTION_WITH_LOCALS(GetPlayerTickets)
	{
		locals.findInput.gameId = input.roundKey.gameId;
		locals.findInput.roundNumber = input.roundKey.roundNumber;
		CALL(FindGameTicketList, locals.findInput, locals.findOutput);
		if (locals.findOutput.returnCode != EReturnCode::SUCCESS)
		{
			output.returnCode = locals.findOutput.returnCode;
			return;
		}
		locals.skipped = 0;
		locals.link = locals.findOutput.firstTicketLink;
		while (locals.link != 0)
		{
			locals.ticket = state.get().tickets.get(locals.link - 1);
			if (locals.ticket.player == input.player)
			{
				++output.totalCount;
				if (locals.skipped++ >= input.offset && output.returnedCount < input.limit && output.returnedCount < output.ticketIndexes.capacity())
				{
					output.ticketIds.set(output.returnedCount, locals.ticket.ticketId);
					output.ticketIndexes.set(output.returnedCount++, locals.link - 1);
				}
			}
			locals.link = locals.ticket.nextLink;
		}
		output.returnCode = EReturnCode::SUCCESS;
	}

	/**
	 * @brief Pages through paid winning ticket ids for one round.
	 * @param input Game id, offset, and requested limit.
	 * @param output Winning indexes, counts, and result code.
	 */
	PUBLIC_FUNCTION_WITH_LOCALS(GetWinners)
	{
		locals.findInput.gameId = input.roundKey.gameId;
		locals.findInput.roundNumber = input.roundKey.roundNumber;
		CALL(FindGameTicketList, locals.findInput, locals.findOutput);
		if (locals.findOutput.returnCode != EReturnCode::SUCCESS)
		{
			output.returnCode = locals.findOutput.returnCode;
			return;
		}
		locals.skipped = 0;
		locals.link = locals.findOutput.firstTicketLink;
		while (locals.link != 0)
		{
			locals.ticket = state.get().tickets.get(locals.link - 1);
			if (locals.ticket.status == ETicketStatus::PAID && locals.ticket.payout > 0)
			{
				++output.totalCount;
				if (locals.skipped++ >= input.offset && output.returnedCount < input.limit && output.returnedCount < output.ticketIndexes.capacity())
				{
					output.ticketIds.set(output.returnedCount, locals.ticket.ticketId);
					output.ticketIndexes.set(output.returnedCount++, locals.link - 1);
				}
			}
			locals.link = locals.ticket.nextLink;
		}
		output.returnCode = EReturnCode::SUCCESS;
	}

	/**
	 * @brief Reads platform governance, Qubic accruals, and registry counters.
	 * @param input Empty input.
	 * @param output Current platform accounting snapshot.
	 */
	PUBLIC_FUNCTION(GetPlatformAccounting)
	{
		output.platformOwner = state.get().platformOwner;
		output.developer1 = state.get().developer1;
		output.developer2 = state.get().developer2;
		output.developer1Accrued = state.get().developer1Accrued;
		output.developer2Accrued = state.get().developer2Accrued;
		output.dividendAccrued = state.get().dividendAccrued;
		output.roundFee = state.get().roundFee;
		output.ticketCount = state.get().ticketCount;
		output.resultCounter = state.get().resultCounter;
		output.activeGameCount = state.get().activeGameCount;
		output.platformFeePercent = state.get().platformFeePercent;
		output.maxCreatorFeePercent = state.get().maxCreatorFeePercent;
	}

	/**
	 * @brief Pages through active generation-aware game ids.
	 * @param input Active-game offset and requested limit.
	 * @param output Active ids, counts, and result code.
	 */
	PUBLIC_FUNCTION_WITH_LOCALS(GetGames)
	{
		locals.skipped = 0;
		output.totalActive = state.get().activeGameCount;
		for (locals.i = 0; locals.i < PLDT_MAX_GAMES; ++locals.i)
		{
			locals.game = state.get().games.get(locals.i);
			if (locals.game.status == EGameStatus::EMPTY_SLOT)
			{
				continue;
			}
			if (locals.skipped++ < input.offset || output.returnedCount >= input.limit || output.returnedCount >= output.gameIds.capacity())
			{
				continue;
			}
			output.gameIds.set(output.returnedCount++, locals.game.gameId);
		}
		output.returnCode = EReturnCode::SUCCESS;
	}

	/**
	 * @brief Sets platform ownership, recipients, and creator-fee limit.
	 * @param input New governance configuration.
	 * @param output Result code.
	 */
	PUBLIC_PROCEDURE(SetPlatformConfig)
	{
		if (state.get().platformOwner != qpi.invocator())
		{
			output.returnCode = EReturnCode::ACCESS_DENIED;
			return;
		}
		if (input.platformOwner == NULL_ID || input.developer1 == NULL_ID || input.developer2 == NULL_ID)
		{
			output.returnCode = EReturnCode::INVALID_VALUE;
			return;
		}
		if (input.roundFee == 0 || input.roundFee > PLDT_MAX_TRANSFER_AMOUNT)
		{
			output.returnCode = EReturnCode::INVALID_VALUE;
			return;
		}
		if (input.maxCreatorFeePercent > PLDT_MAX_CREATOR_FEE_PERCENT)
		{
			output.returnCode = EReturnCode::INVALID_VALUE;
			return;
		}
		state.mut().platformOwner = input.platformOwner;
		state.mut().developer1 = input.developer1;
		state.mut().developer2 = input.developer2;
		state.mut().roundFee = input.roundFee;
		state.mut().maxCreatorFeePercent = input.maxCreatorFeePercent;
		output.returnCode = EReturnCode::SUCCESS;
	}

	/**
	 * @brief Pays accrued Qubic platform revenue to configured recipients.
	 * @param input Empty input.
	 * @param output Amounts paid and result code.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(WithdrawPlatformRevenue)
	{
		if (state.get().platformOwner != qpi.invocator())
		{
			output.returnCode = EReturnCode::ACCESS_DENIED;
			return;
		}
		locals.failed = false;
		if (state.get().developer1Accrued > 0)
		{
			if (state.get().developer1 == NULL_ID)
			{
				locals.failed = true;
			}
			else
			{
				locals.transferResult = qpi.transfer(state.get().developer1, static_cast<sint64>(state.get().developer1Accrued));
				if (locals.transferResult >= 0)
				{
					output.developer1Paid = state.get().developer1Accrued;
					state.mut().developer1Accrued = 0;
				}
				else
				{
					locals.failed = true;
				}
			}
		}
		if (state.get().developer2Accrued > 0)
		{
			if (state.get().developer2 == NULL_ID)
			{
				locals.failed = true;
			}
			else
			{
				locals.transferResult = qpi.transfer(state.get().developer2, static_cast<sint64>(state.get().developer2Accrued));
				if (locals.transferResult >= 0)
				{
					output.developer2Paid = state.get().developer2Accrued;
					state.mut().developer2Accrued = 0;
				}
				else
				{
					locals.failed = true;
				}
			}
		}
		if (state.get().dividendAccrued >= NUMBER_OF_COMPUTORS &&
		    qpi.distributeDividends(static_cast<sint64>(div(state.get().dividendAccrued, static_cast<uint64>(NUMBER_OF_COMPUTORS)))))
		{
			output.dividendPaid =
			    smul(div(state.get().dividendAccrued, static_cast<uint64>(NUMBER_OF_COMPUTORS)), static_cast<uint64>(NUMBER_OF_COMPUTORS));
			state.mut().dividendAccrued -= output.dividendPaid;
		}
		else if (state.get().dividendAccrued >= NUMBER_OF_COMPUTORS)
		{
			locals.failed = true;
		}
		output.returnCode = locals.failed ? EReturnCode::TRANSFER_FAILED : EReturnCode::SUCCESS;
	}

	/**
	 * @brief Atomically validates payment for and persists up to 16 tickets.
	 * @param input Game id, ticket count, and submitted digit arrays.
	 * @param output Accepted generation-aware ticket ids, compatibility slots, and result code.
	 * @note Economics and rounding are applied independently to every ticket.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(BuyTickets)
	{
		// Validate the entire batch and aggregate ledger capacity before accepting any individual ticket.
		if (!isGameIdValid(state, input.gameId))
		{
			locals.refundInput.returnCode = EReturnCode::INVALID_GAME;
			CALL(RefundInvocationReward, locals.refundInput, locals.refundOutput);
			output.returnCode = locals.refundOutput.returnCode;
			return;
		}
		if (input.ticketCount == 0 || input.ticketCount > PLDT_MAX_BATCH_TICKETS)
		{
			locals.refundInput.returnCode = EReturnCode::INVALID_VALUE;
			CALL(RefundInvocationReward, locals.refundInput, locals.refundOutput);
			output.returnCode = locals.refundOutput.returnCode;
			return;
		}
		locals.slot = gameSlot(input.gameId);
		locals.game = state.get().games.get(locals.slot);
		locals.now = qpi.now();
		locals.lifecycleInput.game = locals.game;
		locals.lifecycleInput.now = locals.now;
		CALL(EvaluateGameLifecycle, locals.lifecycleInput, locals.lifecycleOutput);
		if (locals.lifecycleOutput.purchaseReturnCode != EReturnCode::SUCCESS)
		{
			locals.refundInput.returnCode = locals.lifecycleOutput.purchaseReturnCode;
			CALL(RefundInvocationReward, locals.refundInput, locals.refundOutput);
			output.returnCode = locals.refundOutput.returnCode;
			return;
		}
		locals.game.status = locals.lifecycleOutput.effectiveStatus;
		if (locals.game.ticketCount + input.ticketCount > locals.game.ticketLimit ||
		    input.ticketCount > state.get().freeTicketCount + state.get().tickets.capacity() -
		                            (state.get().nextUnusedTicketSlot != 0 ? state.get().nextUnusedTicketSlot : state.get().ticketCount))
		{
			locals.refundInput.returnCode = EReturnCode::TICKET_SOLD_OUT;
			CALL(RefundInvocationReward, locals.refundInput, locals.refundOutput);
			output.returnCode = locals.refundOutput.returnCode;
			return;
		}
		locals.link = locals.game.firstTicketLink;
		locals.playerTickets = 0;
		while (locals.link != 0)
		{
			locals.ticket = state.get().tickets.get(locals.link - 1);
			if (locals.ticket.player == qpi.invocator())
			{
				++locals.playerTickets;
			}
			locals.link = locals.ticket.nextLink;
		}
		if (locals.playerTickets + input.ticketCount > locals.game.playerTicketLimit)
		{
			locals.refundInput.returnCode = EReturnCode::PLAYER_TICKET_LIMIT;
			CALL(RefundInvocationReward, locals.refundInput, locals.refundOutput);
			output.returnCode = locals.refundOutput.returnCode;
			return;
		}
		locals.validateInput.codeLength = locals.game.codeLength;
		locals.validateInput.maxDigit = locals.game.maxDigit;
		locals.validateInput.allowRepeatedDigits = locals.game.allowRepeatedDigits;
		for (locals.i = 0; locals.i < input.ticketCount; ++locals.i)
		{
			locals.validateInput.digits = input.tickets.get(locals.i);
			CALL(ValidateDigits, locals.validateInput, locals.validateOutput);
			if (locals.validateOutput.returnCode != EReturnCode::SUCCESS)
			{
				locals.refundInput.returnCode = locals.validateOutput.returnCode;
				CALL(RefundInvocationReward, locals.refundInput, locals.refundOutput);
				output.returnCode = locals.refundOutput.returnCode;
				return;
			}
		}
		locals.bonusInput.game = locals.game;
		locals.bonusInput.player = qpi.invocator();
		CALL(EvaluateBonusQualification, locals.bonusInput, locals.bonusOutput);
		locals.totalPrice = smul(locals.game.ticketPrice, static_cast<uint64>(input.ticketCount));
		if (locals.totalPrice > PLDT_MAX_TRANSFER_AMOUNT)
		{
			locals.refundInput.returnCode = EReturnCode::INVALID_VALUE;
			CALL(RefundInvocationReward, locals.refundInput, locals.refundOutput);
			output.returnCode = locals.refundOutput.returnCode;
			return;
		}
		locals.economicsInput.ticketPrice = locals.game.ticketPrice;
		locals.economicsInput.creatorFeePercent = locals.game.creatorFeePercent;
		locals.economicsInput.platformFeePercent = state.get().platformFeePercent;
		CALL(CalculateTicketEconomics, locals.economicsInput, locals.economicsOutput);
		if (locals.game.totalRevenue > PLDT_MAX_TRANSFER_AMOUNT - locals.totalPrice ||
		    locals.game.creatorBalance > PLDT_MAX_TRANSFER_AMOUNT - locals.game.prizePool ||
		    smul(locals.economicsOutput.prizeContribution, static_cast<uint64>(input.ticketCount)) >
		        PLDT_MAX_TRANSFER_AMOUNT - locals.game.prizePool - locals.game.creatorBalance)
		{
			locals.refundInput.returnCode = EReturnCode::STORAGE_FULL;
			CALL(RefundInvocationReward, locals.refundInput, locals.refundOutput);
			output.returnCode = locals.refundOutput.returnCode;
			return;
		}
		locals.developer1Fee = smul(locals.economicsOutput.developer1Fee, static_cast<uint64>(input.ticketCount));
		locals.developer2Fee = smul(locals.economicsOutput.developer2Fee, static_cast<uint64>(input.ticketCount));
		locals.dividendFee = smul(locals.economicsOutput.dividendFee, static_cast<uint64>(input.ticketCount));
		// Custody and burn use aggregate amounts, while ticket economics remain rounded per ticket.
		if (locals.game.currencyMode == ECurrencyMode::QUBIC)
		{
			if (state.get().developer1Accrued > PLDT_MAX_TRANSFER_AMOUNT - locals.developer1Fee ||
			    state.get().developer2Accrued > PLDT_MAX_TRANSFER_AMOUNT - locals.developer2Fee ||
			    state.get().dividendAccrued > PLDT_MAX_TRANSFER_AMOUNT - locals.dividendFee)
			{
				locals.refundInput.returnCode = EReturnCode::STORAGE_FULL;
				CALL(RefundInvocationReward, locals.refundInput, locals.refundOutput);
				output.returnCode = locals.refundOutput.returnCode;
				return;
			}
		}
		else
		{
			locals.assetAccounting = state.get().assetAccounting.get(locals.game.assetAccountingLink - 1);
			if (locals.assetAccounting.developer1Accrued > PLDT_MAX_TRANSFER_AMOUNT - locals.developer1Fee ||
			    locals.assetAccounting.developer2Accrued > PLDT_MAX_TRANSFER_AMOUNT - locals.developer2Fee ||
			    locals.assetAccounting.dividendAccrued > PLDT_MAX_TRANSFER_AMOUNT - locals.dividendFee)
			{
				output.returnCode = EReturnCode::STORAGE_FULL;
				return;
			}
		}
		if (locals.game.currencyMode == ECurrencyMode::QUBIC)
		{
			if (qpi.invocationReward() != locals.totalPrice)
			{
				locals.refundInput.returnCode = EReturnCode::TICKET_INVALID_PRICE;
				CALL(RefundInvocationReward, locals.refundInput, locals.refundOutput);
				output.returnCode = locals.refundOutput.returnCode;
				return;
			}
		}
		else
		{
			if (qpi.invocationReward() != 0)
			{
				locals.refundInput.returnCode = EReturnCode::TICKET_INVALID_PRICE;
				CALL(RefundInvocationReward, locals.refundInput, locals.refundOutput);
				output.returnCode = locals.refundOutput.returnCode;
				return;
			}
			locals.possessedShares =
			    qpi.numberOfPossessedShares(locals.game.currencyAsset.assetName, locals.game.currencyAsset.issuer, qpi.invocator(), qpi.invocator(),
			                                locals.game.ownershipManagingContractIndex, locals.game.possessionManagingContractIndex);
			if (locals.possessedShares < static_cast<sint64>(locals.totalPrice))
			{
				output.returnCode = EReturnCode::INSUFFICIENT_FUNDS;
				return;
			}
			locals.transferResult =
			    qpi.transferShareOwnershipAndPossession(locals.game.currencyAsset.assetName, locals.game.currencyAsset.issuer, qpi.invocator(),
			                                            qpi.invocator(), static_cast<sint64>(locals.totalPrice), SELF);
			if (locals.transferResult < 0)
			{
				output.returnCode = EReturnCode::TRANSFER_FAILED;
				return;
			}
		}
		locals.burnInput.game = locals.game;
		locals.burnInput.ticketCount = input.ticketCount;
		CALL(BurnCollectedTicketPayment, locals.burnInput, locals.burnOutput);
		if (locals.burnOutput.returnCode != EReturnCode::SUCCESS)
		{
			locals.transferInput.game = locals.game;
			locals.transferInput.destination = qpi.invocator();
			locals.transferInput.amount = locals.totalPrice;
			CALL(TransferGameCurrency, locals.transferInput, locals.transferOutput);
			output.returnCode = EReturnCode::TRANSFER_FAILED;
			return;
		}
		// Persist every ticket only after the shared custody and burn steps have succeeded.
		state.mut().games.set(locals.slot, locals.game);
		for (locals.i = 0; locals.i < input.ticketCount; ++locals.i)
		{
			locals.applyInput.gameId = input.gameId;
			locals.applyInput.digits = input.tickets.get(locals.i);
			locals.applyInput.bonusQualified = locals.bonusOutput.qualified;
			CALL(ApplyAcceptedTicket, locals.applyInput, locals.applyOutput);
			if (locals.applyOutput.returnCode != EReturnCode::SUCCESS)
			{
				output.returnCode = locals.applyOutput.returnCode;
				return;
			}
			output.ticketIds.set(output.acceptedCount, locals.applyOutput.ticketId);
			output.ticketIndexes.set(output.acceptedCount++, locals.applyOutput.ticketIndex);
		}
		output.returnCode = EReturnCode::SUCCESS;
	}

	/**
	 * @brief Releases caller-owned managed shares to another managing contract.
	 * @param input Asset, share count, and destination managing contract.
	 * @param output Release fee result and status code.
	 */
	PUBLIC_PROCEDURE(TransferShareManagementRights)
	{
		output.transferResult = qpi.releaseShares(input.asset, qpi.invocator(), qpi.invocator(), input.numberOfShares, input.newManagingContractIndex,
		                                          input.newManagingContractIndex, qpi.invocationReward());
		output.returnCode = output.transferResult >= 0 ? EReturnCode::SUCCESS : EReturnCode::TRANSFER_FAILED;
	}

	/**
	 * @brief Pays platform accruals for one managed asset accounting bucket.
	 * @param input Asset issuance and its managing contract indexes.
	 * @param output Developer and shareholder amounts paid plus result code.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(WithdrawAssetPlatformRevenue)
	{
		if (state.get().platformOwner != qpi.invocator())
		{
			output.returnCode = EReturnCode::ACCESS_DENIED;
			return;
		}
		locals.found = false;
		locals.failed = false;
		for (locals.i = 0; locals.i < state.get().assetAccounting.capacity(); ++locals.i)
		{
			locals.accounting = state.get().assetAccounting.get(locals.i);
			if (locals.accounting.isActive && locals.accounting.asset.assetName == input.asset.assetName &&
			    locals.accounting.asset.issuer == input.asset.issuer &&
			    locals.accounting.ownershipManagingContractIndex == input.ownershipManagingContractIndex &&
			    locals.accounting.possessionManagingContractIndex == input.possessionManagingContractIndex)
			{
				locals.found = true;
				break;
			}
		}
		if (!locals.found)
		{
			output.returnCode = EReturnCode::INVALID_VALUE;
			return;
		}
		if (locals.accounting.developer1Accrued > 0)
		{
			if (state.get().developer1 == NULL_ID)
			{
				locals.failed = true;
			}
			else
			{
				locals.transferResult =
				    qpi.transferShareOwnershipAndPossession(locals.accounting.asset.assetName, locals.accounting.asset.issuer, SELF, SELF,
				                                            static_cast<sint64>(locals.accounting.developer1Accrued), state.get().developer1);
				if (locals.transferResult >= 0)
				{
					output.developer1Paid = locals.accounting.developer1Accrued;
					locals.accounting.developer1Accrued = 0;
				}
				else
				{
					locals.failed = true;
				}
			}
		}
		if (locals.accounting.developer2Accrued > 0)
		{
			if (state.get().developer2 == NULL_ID)
			{
				locals.failed = true;
			}
			else
			{
				locals.transferResult =
				    qpi.transferShareOwnershipAndPossession(locals.accounting.asset.assetName, locals.accounting.asset.issuer, SELF, SELF,
				                                            static_cast<sint64>(locals.accounting.developer2Accrued), state.get().developer2);
				if (locals.transferResult >= 0)
				{
					output.developer2Paid = locals.accounting.developer2Accrued;
					locals.accounting.developer2Accrued = 0;
				}
				else
				{
					locals.failed = true;
				}
			}
		}
		locals.dividendInput.dividendAsset = locals.accounting.asset;
		locals.dividendInput.dividendAmount = locals.accounting.dividendAccrued;
		CALL(TransferAssetDividend, locals.dividendInput, locals.dividendOutput);
		output.dividendPaid = locals.dividendOutput.distributedAmount;
		locals.accounting.dividendAccrued -= output.dividendPaid;
		if (locals.dividendOutput.failed)
		{
			locals.failed = true;
		}
		if (locals.accounting.activeGameCount == 0 && locals.accounting.developer1Accrued == 0 && locals.accounting.developer2Accrued == 0 &&
		    locals.accounting.dividendAccrued == 0)
		{
			setMemory(locals.accounting, 0);
		}
		state.mut().assetAccounting.set(locals.i, locals.accounting);
		output.returnCode = locals.failed ? EReturnCode::TRANSFER_FAILED : EReturnCode::SUCCESS;
	}

private:
	PRIVATE_FUNCTION(CalculateTicketEconomics)
	{
		output.platformFee = mulDiv(input.ticketPrice, input.platformFeePercent, 100ULL);
		output.net = input.ticketPrice - output.platformFee;
		output.creatorFee = mulDiv(output.net, input.creatorFeePercent, 100ULL);
		output.burn = mulDiv(output.net, PLDT_BURN_PERCENT, 100ULL);
		output.prizeContribution = output.net - output.creatorFee - output.burn;
		output.developer1Fee = mulDiv(output.platformFee, PLDT_PLATFORM_DEV1_SHARE_PERCENT, 100ULL);
		output.developer2Fee = mulDiv(output.platformFee, PLDT_PLATFORM_DEV2_SHARE_PERCENT, 100ULL);
		output.dividendFee = output.platformFee - output.developer1Fee - output.developer2Fee;
	}

	PRIVATE_FUNCTION(EvaluateGameLifecycle)
	{
		output.effectiveStatus = input.game.status;
		output.action = EGameLifecycleAction::NONE;
		output.purchaseReturnCode = EReturnCode::GAME_CLOSED;
		output.canCancel = false;
		if (input.game.status == EGameStatus::SCHEDULED)
		{
			if (input.now < input.game.startAt)
			{
				output.purchaseReturnCode = EReturnCode::GAME_NOT_STARTED;
				output.canCancel = true;
				return;
			}
			if (input.now < input.game.drawAt)
			{
				output.effectiveStatus = EGameStatus::SELLING;
				output.action = EGameLifecycleAction::OPEN_SALES;
				output.purchaseReturnCode = EReturnCode::SUCCESS;
				return;
			}
			output.effectiveStatus = EGameStatus::CLOSED;
			output.action = input.game.ticketCount == 0 ? EGameLifecycleAction::FINALIZE_NO_TICKETS : EGameLifecycleAction::BEGIN_SETTLEMENT;
			return;
		}
		if (input.game.status == EGameStatus::SELLING)
		{
			if (input.now < input.game.drawAt)
			{
				output.purchaseReturnCode = EReturnCode::SUCCESS;
				return;
			}
			output.effectiveStatus = EGameStatus::CLOSED;
			output.action = input.game.ticketCount == 0 ? EGameLifecycleAction::FINALIZE_NO_TICKETS : EGameLifecycleAction::BEGIN_SETTLEMENT;
			return;
		}
		if (input.game.status == EGameStatus::CLOSED)
		{
			output.action = EGameLifecycleAction::BEGIN_SETTLEMENT;
		}
	}

	PRIVATE_FUNCTION_WITH_LOCALS(EvaluateBonusQualification)
	{
		output.qualified = false;
		for (locals.i = 0; locals.i < input.game.bonusAssetCount; ++locals.i)
		{
			locals.possessedShares = qpi.numberOfPossessedShares(
			    input.game.bonusAssets.get(locals.i).assetName, input.game.bonusAssets.get(locals.i).issuer, input.player, input.player,
			    input.game.bonusOwnershipManagingContractIndex, input.game.bonusPossessionManagingContractIndex);
			if (locals.possessedShares > 0)
			{
				output.qualified = true;
				return;
			}
		}
	}

	PRIVATE_FUNCTION_WITH_LOCALS(FindGameTicketList)
	{
		locals.foundGame = false;
		if (input.gameId == 0 || input.roundNumber == 0)
		{
			output.returnCode = EReturnCode::INVALID_ROUND;
			return;
		}
		if (isGameIdValid(state, input.gameId) && state.get().games.get(gameSlot(input.gameId)).roundNumber == input.roundNumber)
		{
			output.firstTicketLink = state.get().games.get(gameSlot(input.gameId)).firstTicketLink;
			output.returnCode = EReturnCode::SUCCESS;
			return;
		}
		locals.available = state.get().resultCounter < PLDT_RESULT_HISTORY_SIZE ? state.get().resultCounter : PLDT_RESULT_HISTORY_SIZE;
		for (locals.i = 0; locals.i < locals.available; ++locals.i)
		{
			locals.counter = state.get().resultCounter - 1 - locals.i;
			locals.result = state.get().results.get(static_cast<uint16>(mod(locals.counter, static_cast<uint64>(PLDT_RESULT_STORAGE_SIZE))));
			if (locals.result.gameId == input.gameId)
			{
				locals.foundGame = true;
				if (locals.result.roundNumber != input.roundNumber)
				{
					continue;
				}
				if (!locals.result.detailsAvailable)
				{
					output.returnCode = EReturnCode::HISTORY_EXPIRED;
					return;
				}
				output.firstTicketLink = locals.result.firstTicketLink;
				output.returnCode = EReturnCode::SUCCESS;
				return;
			}
		}
		output.returnCode = locals.foundGame || isGameIdValid(state, input.gameId) ? EReturnCode::INVALID_ROUND : EReturnCode::INVALID_GAME;
	}

	PRIVATE_PROCEDURE(RefundInvocationReward)
	{
		output.returnCode = input.returnCode;
		if (qpi.invocationReward() > 0 && qpi.transfer(qpi.invocator(), qpi.invocationReward()) < 0)
		{
			output.returnCode = EReturnCode::TRANSFER_FAILED;
		}
	}

	PRIVATE_PROCEDURE(TransferGameCurrency)
	{
		output.transferResult = 0;
		if (input.amount == 0)
		{
			return;
		}
		if (input.game.currencyMode == ECurrencyMode::QUBIC)
		{
			output.transferResult = qpi.transfer(input.destination, static_cast<sint64>(input.amount));
			return;
		}
		output.transferResult = qpi.transferShareOwnershipAndPossession(input.game.currencyAsset.assetName, input.game.currencyAsset.issuer, SELF,
		                                                                SELF, static_cast<sint64>(input.amount), input.destination);
	}

	PRIVATE_PROCEDURE_WITH_LOCALS(BurnCollectedTicketPayment)
	{
		locals.economicsInput.ticketPrice = input.game.ticketPrice;
		locals.economicsInput.creatorFeePercent = input.game.creatorFeePercent;
		locals.economicsInput.platformFeePercent = state.get().platformFeePercent;
		CALL(CalculateTicketEconomics, locals.economicsInput, locals.economicsOutput);
		locals.totalBurn = smul(locals.economicsOutput.burn, static_cast<uint64>(input.ticketCount));
		if (locals.totalBurn == 0)
		{
			output.returnCode = EReturnCode::SUCCESS;
			return;
		}
		if (input.game.currencyMode == ECurrencyMode::QUBIC)
		{
			output.returnCode = qpi.burn(static_cast<sint64>(locals.totalBurn)) < 0 ? EReturnCode::TRANSFER_FAILED : EReturnCode::SUCCESS;
			return;
		}
		locals.transferInput.game = input.game;
		locals.transferInput.destination = NULL_ID;
		locals.transferInput.amount = locals.totalBurn;
		CALL(TransferGameCurrency, locals.transferInput, locals.transferOutput);
		output.returnCode = locals.transferOutput.transferResult < 0 ? EReturnCode::TRANSFER_FAILED : EReturnCode::SUCCESS;
	}

	PRIVATE_PROCEDURE_WITH_LOCALS(ApplyAcceptedTicket)
	{
		// Preconditions are validated for the complete purchase before burn, so this commit path has no fallible transfers.
		locals.slot = gameSlot(input.gameId);
		locals.game = state.get().games.get(locals.slot);
		locals.economicsInput.ticketPrice = locals.game.ticketPrice;
		locals.economicsInput.creatorFeePercent = locals.game.creatorFeePercent;
		locals.economicsInput.platformFeePercent = state.get().platformFeePercent;
		CALL(CalculateTicketEconomics, locals.economicsInput, locals.economicsOutput);
		output.prizeContribution = locals.economicsOutput.prizeContribution;
		if (locals.game.currencyMode == ECurrencyMode::QUBIC)
		{
			state.mut().developer1Accrued = sadd(state.get().developer1Accrued, locals.economicsOutput.developer1Fee);
			state.mut().developer2Accrued = sadd(state.get().developer2Accrued, locals.economicsOutput.developer2Fee);
			state.mut().dividendAccrued = sadd(state.get().dividendAccrued, locals.economicsOutput.dividendFee);
		}
		else
		{
			locals.assetAccounting = state.get().assetAccounting.get(locals.game.assetAccountingLink - 1);
			locals.assetAccounting.developer1Accrued = sadd(locals.assetAccounting.developer1Accrued, locals.economicsOutput.developer1Fee);
			locals.assetAccounting.developer2Accrued = sadd(locals.assetAccounting.developer2Accrued, locals.economicsOutput.developer2Fee);
			locals.assetAccounting.dividendAccrued = sadd(locals.assetAccounting.dividendAccrued, locals.economicsOutput.dividendFee);
			state.mut().assetAccounting.set(locals.game.assetAccountingLink - 1, locals.assetAccounting);
		}
		locals.game.creatorRevenue = sadd(locals.game.creatorRevenue, locals.economicsOutput.creatorFee);
		locals.game.prizePool = sadd(locals.game.prizePool, output.prizeContribution);
		locals.game.totalRevenue = sadd(locals.game.totalRevenue, locals.game.ticketPrice);
		setMemory(locals.ticket, 0);
		locals.ticket.digits = input.digits;
		locals.ticket.player = qpi.invocator();
		locals.ticket.gameId = input.gameId;
		locals.ticket.bonusQualified = input.bonusQualified;
		locals.ticket.status = ETicketStatus::ACTIVE;
		if (state.get().freeTicketHead != 0)
		{
			locals.ticketSlot = state.get().freeTicketHead - 1;
			locals.previousTicket = state.get().tickets.get(locals.ticketSlot);
			state.mut().freeTicketHead = locals.previousTicket.nextLink;
			state.mut().freeTicketCount = state.get().freeTicketCount - 1;
			locals.ticketGeneration = (locals.previousTicket.ticketId >> PLDT_TICKET_SLOT_BITS) + 1;
		}
		else
		{
			locals.ticketSlot = state.get().nextUnusedTicketSlot != 0 ? state.get().nextUnusedTicketSlot : state.get().ticketCount;
			locals.ticketGeneration = 1;
			state.mut().nextUnusedTicketSlot = locals.ticketSlot + 1;
		}
		output.ticketIndex = locals.ticketSlot;
		output.ticketId = (locals.ticketGeneration << PLDT_TICKET_SLOT_BITS) | locals.ticketSlot;
		locals.ticket.ticketId = output.ticketId;
		state.mut().tickets.set(locals.ticketSlot, locals.ticket);
		if (locals.game.lastTicketLink != 0)
		{
			locals.previousTicket = state.get().tickets.get(locals.game.lastTicketLink - 1);
			locals.previousTicket.nextLink = output.ticketIndex + 1;
			state.mut().tickets.set(locals.game.lastTicketLink - 1, locals.previousTicket);
		}
		else
		{
			locals.game.firstTicketLink = output.ticketIndex + 1;
		}
		locals.game.lastTicketLink = output.ticketIndex + 1;
		++locals.game.ticketCount;
		state.mut().ticketCount = state.get().ticketCount + 1;
		if (locals.game.ticketCount >= locals.game.ticketLimit)
		{
			locals.game.status = EGameStatus::CLOSED;
		}
		state.mut().games.set(locals.slot, locals.game);
		output.returnCode = EReturnCode::SUCCESS;
	}

	PRIVATE_PROCEDURE_WITH_LOCALS(TransferAssetDividend)
	{
		output.distributedAmount = 0;
		output.failed = false;
		if (input.dividendAmount == 0)
		{
			return;
		}
		locals.shareholdersAsset.issuer = id::zero();
		locals.shareholdersAsset.assetName = PLDT_CONTRACT_ASSET_NAME;
		locals.dividendPerShare = static_cast<sint64>(div(input.dividendAmount, static_cast<uint64>(NUMBER_OF_COMPUTORS)));
		locals.remainder = mod(input.dividendAmount, static_cast<uint64>(NUMBER_OF_COMPUTORS));
		locals.targetDistribution = input.dividendAmount;
		locals.shareholdersIter.begin(locals.shareholdersAsset);
		while (!locals.shareholdersIter.reachedEnd())
		{
			locals.holderShares = locals.shareholdersIter.numberOfPossessedShares();
			if (locals.holderShares > 0)
			{
				locals.holderDividend = smul(locals.holderShares, locals.dividendPerShare);
				locals.holderRemainder =
				    static_cast<uint64>(locals.holderShares) < locals.remainder ? static_cast<uint64>(locals.holderShares) : locals.remainder;
				locals.holderDividend = static_cast<sint64>(sadd(static_cast<uint64>(locals.holderDividend), locals.holderRemainder));
				locals.remainder -= locals.holderRemainder;
				if (locals.holderDividend > 0)
				{
					locals.transferResult = qpi.transferShareOwnershipAndPossession(input.dividendAsset.assetName, input.dividendAsset.issuer, SELF,
					                                                                SELF, locals.holderDividend, locals.shareholdersIter.possessor());
					if (locals.transferResult >= 0)
					{
						output.distributedAmount = sadd(output.distributedAmount, static_cast<uint64>(locals.holderDividend));
					}
					else
					{
						output.failed = true;
					}
				}
			}
			locals.shareholdersIter.next();
		}
		if (output.distributedAmount != locals.targetDistribution)
		{
			output.failed = true;
		}
	}

	PRIVATE_FUNCTION_WITH_LOCALS(CountMatches)
	{
		setMemory(locals.playerCounts, 0);
		setMemory(locals.winningCounts, 0);
		output.tierIndex = 0;
		output.exact = 0;
		output.misplaced = 0;
		for (locals.i = 0; locals.i < input.codeLength; ++locals.i)
		{
			locals.playerDigit = input.playerDigits.get(locals.i);
			locals.winningDigit = input.winningDigits.get(locals.i);
			if (locals.playerDigit == locals.winningDigit)
			{
				++output.exact;
			}
			else
			{
				locals.playerCounts.set(locals.playerDigit, locals.playerCounts.get(locals.playerDigit) + 1);
				locals.winningCounts.set(locals.winningDigit, locals.winningCounts.get(locals.winningDigit) + 1);
			}
		}
		for (locals.i = 0; locals.i <= PLDT_MAX_DIGIT; ++locals.i)
		{
			locals.playerCount = locals.playerCounts.get(locals.i);
			locals.winningCount = locals.winningCounts.get(locals.i);
			output.misplaced += locals.playerCount < locals.winningCount ? locals.playerCount : locals.winningCount;
		}
		output.tierIndex = payoutMatrixIndex(output.exact, output.misplaced);
	}

	PRIVATE_FUNCTION_WITH_LOCALS(GenerateWinningDigits)
	{
		setMemory(locals.used, 0);
		setMemory(output.digits, 0);
		for (locals.index = 0; locals.index < input.codeLength; ++locals.index)
		{
			locals.value = input.seed ^ (0x9e3779b97f4a7c15ULL * (locals.index + 1));
			locals.value ^= locals.value >> 30;
			locals.value *= 0xbf58476d1ce4e5b9ULL;
			locals.value ^= locals.value >> 27;
			locals.value *= 0x94d049bb133111ebULL;
			locals.value ^= locals.value >> 31;
			locals.candidate = static_cast<uint8>(mod(locals.value, static_cast<uint64>(input.maxDigit + 1)));
			locals.attempts = 0;
			while (!input.allowRepeatedDigits && locals.used.get(locals.candidate) != 0 && locals.attempts < PLDT_RANDOM_RETRY_LIMIT)
			{
				++locals.attempts;
				locals.value ^= locals.value << 13;
				locals.value ^= locals.value >> 7;
				locals.value ^= locals.value << 17;
				locals.candidate = static_cast<uint8>(mod(locals.value, static_cast<uint64>(input.maxDigit + 1)));
			}
			if (!input.allowRepeatedDigits && locals.used.get(locals.candidate) != 0)
			{
				for (locals.fallback = 0; locals.fallback <= input.maxDigit; ++locals.fallback)
				{
					if (locals.used.get(locals.fallback) == 0)
					{
						locals.candidate = locals.fallback;
						break;
					}
				}
			}
			output.digits.set(locals.index, locals.candidate);
			locals.used.set(locals.candidate, 1);
		}
	}

	PRIVATE_PROCEDURE_WITH_LOCALS(BeginSettlement)
	{
		// Snapshot immutable round inputs before any budgeted ticket classification begins.
		locals.game = state.get().games.get(input.slot);
		if (locals.game.status != EGameStatus::CLOSED)
		{
			output.returnCode = EReturnCode::INVALID_STATE;
			return;
		}
		setMemory(locals.progress, 0);
		locals.randomData.prevSpectrumDigest = qpi.getPrevSpectrumDigest();
		locals.randomData.gameId = locals.game.gameId;
		locals.randomData.roundNumber = locals.game.roundNumber;
		locals.randomData.ticketCount = locals.game.ticketCount;
		locals.seed = qpi.K12(locals.randomData).u64._0;
		locals.generateInput.seed = locals.seed;
		locals.generateInput.codeLength = locals.game.codeLength;
		locals.generateInput.maxDigit = locals.game.maxDigit;
		locals.generateInput.allowRepeatedDigits = locals.game.allowRepeatedDigits;
		CALL(GenerateWinningDigits, locals.generateInput, locals.generateOutput);
		locals.progress.winningDigits = locals.generateOutput.digits;
		locals.progress.cursorLink = locals.game.firstTicketLink;
		locals.progress.prizePoolSnapshot = locals.game.prizePool;
		locals.game.status = EGameStatus::COUNTING;
		state.mut().settlements.set(input.slot, locals.progress);
		state.mut().games.set(input.slot, locals.game);
		output.returnCode = EReturnCode::SUCCESS;
	}

	PRIVATE_PROCEDURE_WITH_LOCALS(AdvanceSettlement)
	{
		locals.slot = input.slot;
		locals.game = state.get().games.get(locals.slot);
		locals.progress = state.get().settlements.get(locals.slot);
		locals.budget = input.actionBudget;
		// Phase 1 classifies tickets and persists the cursor so work can resume on a later tick.
		while (locals.budget > 0 && locals.game.status == EGameStatus::COUNTING && locals.progress.cursorLink != 0)
		{
			locals.link = locals.progress.cursorLink;
			locals.ticket = state.get().tickets.get(locals.link - 1);
			locals.matchInput.playerDigits = locals.ticket.digits;
			locals.matchInput.winningDigits = locals.progress.winningDigits;
			locals.matchInput.codeLength = locals.game.codeLength;
			CALL(CountMatches, locals.matchInput, locals.matchOutput);
			locals.ticket.exact = locals.matchOutput.exact;
			locals.ticket.misplaced = locals.matchOutput.misplaced;
			locals.ticket.tierIndex = locals.matchOutput.tierIndex;
			if (locals.game.tierWeightsBps.get(locals.ticket.tierIndex) > 0)
			{
				locals.ticket.winnerWeight = locals.ticket.bonusQualified ? locals.game.bonusMultiplierBps : PLDT_BONUS_MULTIPLIER_SCALE;
				locals.progress.tierWinnerCount.set(locals.ticket.tierIndex, locals.progress.tierWinnerCount.get(locals.ticket.tierIndex) + 1);
				if (locals.ticket.bonusQualified)
				{
					locals.progress.tierBonusCount.set(locals.ticket.tierIndex, locals.progress.tierBonusCount.get(locals.ticket.tierIndex) + 1);
				}
				++locals.progress.winnerCount;
			}
			else
			{
				locals.ticket.status = ETicketStatus::LOST;
			}
			locals.progress.cursorLink = locals.ticket.nextLink;
			state.mut().tickets.set(locals.link - 1, locals.ticket);
			--locals.budget;
		}
		// Phase 2 allocates the complete pool only across populated tiers using largest-remainder rounding.
		if (locals.game.status == EGameStatus::COUNTING && locals.progress.cursorLink == 0)
		{
			if (locals.progress.winnerCount == 0)
			{
				state.mut().settlements.set(locals.slot, locals.progress);
				locals.finalizeInput.slot = locals.slot;
				locals.finalizeInput.reason = EGameTerminalReason::NO_WINNERS;
				CALL(FinalizeGame, locals.finalizeInput, locals.finalizeOutput);
				output.actionsUsed = input.actionBudget - locals.budget;
				output.returnCode = locals.finalizeOutput.returnCode;
				return;
			}
			locals.progress.activeTierBps = 0;
			for (locals.i = 0; locals.i < PLDT_TIER_CAPACITY; ++locals.i)
			{
				if (locals.progress.tierWinnerCount.get(static_cast<uint16>(locals.i)) > 0)
				{
					locals.progress.activeTierBps += locals.game.tierWeightsBps.get(static_cast<uint16>(locals.i));
				}
			}
			locals.progress.allocatedPool = 0;
			for (locals.i = 0; locals.i < PLDT_TIER_CAPACITY; ++locals.i)
			{
				if (locals.progress.tierWinnerCount.get(static_cast<uint16>(locals.i)) == 0)
				{
					continue;
				}
				locals.tierPool = mulDiv(locals.progress.prizePoolSnapshot, locals.game.tierWeightsBps.get(static_cast<uint16>(locals.i)),
				                         locals.progress.activeTierBps);
				locals.fraction = mulMod(locals.progress.prizePoolSnapshot, locals.game.tierWeightsBps.get(static_cast<uint16>(locals.i)),
				                         locals.progress.activeTierBps);
				locals.progress.tierPools.set(static_cast<uint16>(locals.i), locals.tierPool);
				locals.progress.tierFractions.set(static_cast<uint16>(locals.i), locals.fraction);
				locals.progress.allocatedPool = sadd(locals.progress.allocatedPool, locals.tierPool);
			}
			locals.remainder = locals.progress.prizePoolSnapshot - locals.progress.allocatedPool;
			while (locals.remainder > 0)
			{
				locals.foundTier = false;
				locals.bestFraction = 0;
				locals.bestTier = 0;
				for (locals.i = 0; locals.i < PLDT_TIER_CAPACITY; ++locals.i)
				{
					locals.fraction = locals.progress.tierFractions.get(static_cast<uint16>(locals.i));
					if (locals.progress.tierWinnerCount.get(static_cast<uint16>(locals.i)) > 0 && locals.fraction != PLDT_UINT64_SENTINEL &&
					    (!locals.foundTier || locals.fraction > locals.bestFraction))
					{
						locals.foundTier = true;
						locals.bestFraction = locals.fraction;
						locals.bestTier = static_cast<uint16>(locals.i);
					}
				}
				locals.progress.tierPools.set(locals.bestTier, locals.progress.tierPools.get(locals.bestTier) + 1);
				locals.progress.tierFractions.set(locals.bestTier, PLDT_UINT64_SENTINEL);
				--locals.remainder;
			}
			for (locals.i = 0; locals.i < PLDT_TIER_CAPACITY; ++locals.i)
			{
				locals.count = locals.progress.tierWinnerCount.get(static_cast<uint16>(locals.i));
				if (locals.count == 0)
				{
					continue;
				}
				locals.bonusCount = locals.progress.tierBonusCount.get(static_cast<uint16>(locals.i));
				locals.totalWeight = smul(locals.count - locals.bonusCount, static_cast<uint64>(PLDT_BONUS_MULTIPLIER_SCALE));
				locals.totalWeight = sadd(locals.totalWeight, smul(locals.bonusCount, static_cast<uint64>(locals.game.bonusMultiplierBps)));
				locals.tierPool = locals.progress.tierPools.get(static_cast<uint16>(locals.i));
				locals.progress.normalPayout.set(static_cast<uint16>(locals.i),
				                                 mulDiv(locals.tierPool, PLDT_BONUS_MULTIPLIER_SCALE, locals.totalWeight));
				locals.progress.bonusPayout.set(static_cast<uint16>(locals.i),
				                                mulDiv(locals.tierPool, locals.game.bonusMultiplierBps, locals.totalWeight));
				locals.floorSum = smul(locals.count - locals.bonusCount, locals.progress.normalPayout.get(static_cast<uint16>(locals.i)));
				locals.floorSum = sadd(locals.floorSum, smul(locals.bonusCount, locals.progress.bonusPayout.get(static_cast<uint16>(locals.i))));
				locals.progress.tierRemainder.set(static_cast<uint16>(locals.i), locals.tierPool - locals.floorSum);
			}
			locals.progress.cursorLink = locals.game.firstTicketLink;
			locals.game.winnerCount = locals.progress.winnerCount;
			locals.game.status = EGameStatus::PAYING;
		}
		// Phase 3 transfers bounded winner payouts and advances only after each transfer succeeds.
		while (locals.budget > 0 && locals.game.status == EGameStatus::PAYING && locals.progress.cursorLink != 0)
		{
			locals.link = locals.progress.cursorLink;
			locals.ticket = state.get().tickets.get(locals.link - 1);
			if (locals.ticket.winnerWeight == 0)
			{
				locals.ticket.status = ETicketStatus::LOST;
			}
			else
			{
				locals.payout = locals.ticket.bonusQualified ? locals.progress.bonusPayout.get(locals.ticket.tierIndex)
				                                             : locals.progress.normalPayout.get(locals.ticket.tierIndex);
				if (locals.progress.tierRemainder.get(locals.ticket.tierIndex) > 0)
				{
					++locals.payout;
				}
				locals.transferInput.game = locals.game;
				locals.transferInput.destination = locals.ticket.player;
				locals.transferInput.amount = locals.payout;
				CALL(TransferGameCurrency, locals.transferInput, locals.transferOutput);
				if (locals.transferOutput.transferResult < 0)
				{
					state.mut().settlements.set(locals.slot, locals.progress);
					state.mut().games.set(locals.slot, locals.game);
					output.actionsUsed = input.actionBudget - locals.budget;
					output.returnCode = EReturnCode::TRANSFER_FAILED;
					return;
				}
				locals.ticket.payout = locals.payout;
				locals.ticket.status = ETicketStatus::PAID;
				locals.game.totalPaid = sadd(locals.game.totalPaid, locals.payout);
				if (locals.progress.tierRemainder.get(locals.ticket.tierIndex) > 0)
				{
					locals.progress.tierRemainder.set(locals.ticket.tierIndex, locals.progress.tierRemainder.get(locals.ticket.tierIndex) - 1);
				}
			}
			locals.progress.cursorLink = locals.ticket.nextLink;
			state.mut().tickets.set(locals.link - 1, locals.ticket);
			--locals.budget;
		}
		// A settled round must distribute the snapshot exactly before terminal accounting can begin.
		state.mut().settlements.set(locals.slot, locals.progress);
		state.mut().games.set(locals.slot, locals.game);
		if (locals.game.status == EGameStatus::PAYING && locals.progress.cursorLink == 0)
		{
			if (locals.game.totalPaid != locals.progress.prizePoolSnapshot)
			{
				output.actionsUsed = input.actionBudget - locals.budget;
				output.returnCode = EReturnCode::UNKNOWN_ERROR;
				return;
			}
			locals.finalizeInput.slot = locals.slot;
			locals.finalizeInput.reason = EGameTerminalReason::SETTLED;
			CALL(FinalizeGame, locals.finalizeInput, locals.finalizeOutput);
			output.actionsUsed = input.actionBudget - locals.budget;
			output.returnCode = locals.finalizeOutput.returnCode;
			return;
		}
		output.actionsUsed = input.actionBudget - locals.budget;
		output.returnCode = EReturnCode::SUCCESS;
	}

	PRIVATE_PROCEDURE_WITH_LOCALS(FinalizeGame)
	{
		// Reserve a result slot before transfers so unreclaimed ticket chains can never become unreachable.
		locals.game = state.get().games.get(input.slot);
		locals.progress = state.get().settlements.get(input.slot);
		locals.resultIndex = mod(state.get().resultCounter, static_cast<uint64>(PLDT_RESULT_STORAGE_SIZE));
		locals.previousResult = state.get().results.get(static_cast<uint16>(locals.resultIndex));
		if (locals.previousResult.resultSequence != 0 && locals.previousResult.detailsAvailable && locals.previousResult.firstTicketLink != 0)
		{
			// Preserve the only link to unreclaimed tickets; background reclamation will unblock finalization.
			output.returnCode = EReturnCode::STORAGE_FULL;
			return;
		}
		// Finalization is resumable for both modes: ledgers are frozen once, then pending transfers are retried safely.
		if (locals.game.status != EGameStatus::FINALIZING)
		{
			locals.game.finalizingRoundReason = input.reason;
			locals.game.finalizingStopReason = EGameStopReason::NONE;
			if (input.reason == EGameTerminalReason::NO_TICKETS || input.reason == EGameTerminalReason::NO_WINNERS ||
			    input.reason == EGameTerminalReason::OWNER_CANCELLED)
			{
				// The purchase and funding guards preserve creatorBalance + prizePool <= MAX_AMOUNT.
				locals.game.creatorBalance = sadd(locals.game.creatorBalance, locals.game.prizePool);
			}
			if (locals.game.creatorRevenueMode == ECreatorRevenueMode::REINVEST)
			{
				locals.availableCredit = PLDT_MAX_TRANSFER_AMOUNT - locals.game.creatorBalance;
				locals.amountToCredit = locals.game.creatorRevenue < locals.availableCredit ? locals.game.creatorRevenue : locals.availableCredit;
				locals.game.creatorBalance = sadd(locals.game.creatorBalance, locals.amountToCredit);
				locals.game.pendingCreatorCurrencyPayout =
				    sadd(locals.game.pendingCreatorCurrencyPayout, locals.game.creatorRevenue - locals.amountToCredit);
			}
			else
			{
				locals.game.pendingCreatorCurrencyPayout = sadd(locals.game.pendingCreatorCurrencyPayout, locals.game.creatorRevenue);
			}
			locals.game.creatorRevenue = 0;
			if (locals.game.stopRequested || input.reason == EGameTerminalReason::OWNER_CANCELLED)
			{
				locals.game.finalizingStopReason = EGameStopReason::OWNER_REQUESTED;
			}
			else if (locals.game.mode == EGameMode::ONE_SHOT)
			{
				locals.game.finalizingStopReason = EGameStopReason::ONE_SHOT_COMPLETE;
			}
			else
			{
				if (locals.game.pendingEconomics.isSet)
				{
					locals.game.ticketPrice = locals.game.pendingEconomics.ticketPrice;
					locals.game.creatorPrizeSeed = locals.game.pendingEconomics.creatorPrizeSeed;
					locals.game.ticketLimit = locals.game.pendingEconomics.ticketLimit;
					locals.game.playerTicketLimit = locals.game.pendingEconomics.playerTicketLimit;
					locals.game.creatorFeePercent = locals.game.pendingEconomics.creatorFeePercent;
					locals.game.creatorRevenueMode = locals.game.pendingEconomics.creatorRevenueMode;
					setMemory(locals.game.pendingEconomics, 0);
				}
				if (locals.game.runCredit < locals.game.roundFeeSnapshot || locals.game.creatorBalance < locals.game.creatorPrizeSeed)
				{
					locals.game.finalizingStopReason = EGameStopReason::OUT_OF_FUNDS;
				}
			}
			if (locals.game.finalizingStopReason != EGameStopReason::NONE)
			{
				locals.game.pendingRunCreditPayout = locals.game.runCredit;
				locals.game.runCredit = 0;
				locals.game.pendingCreatorBalancePayout = locals.game.creatorBalance;
				locals.game.creatorBalance = 0;
			}
			locals.game.status = EGameStatus::FINALIZING;
			state.mut().games.set(input.slot, locals.game);
		}
		if (locals.game.finalizingStopReason == EGameStopReason::NONE)
		{
			locals.nextStartAt = qpi.now();
			locals.nextDrawAt = locals.nextStartAt;
			if (!locals.nextDrawAt.addMicrosec(static_cast<sint64>(locals.game.roundDurationMicroseconds)))
			{
				locals.game.finalizingStopReason = EGameStopReason::SCHEDULE_EXHAUSTED;
				locals.game.pendingRunCreditPayout = locals.game.runCredit;
				locals.game.runCredit = 0;
				locals.game.pendingCreatorBalancePayout = locals.game.creatorBalance;
				locals.game.creatorBalance = 0;
				state.mut().games.set(input.slot, locals.game);
			}
		}
		// Drain owner payouts individually; a failed transfer leaves its pending ledger intact for the next tick.
		if (locals.game.pendingCreatorCurrencyPayout > 0)
		{
			locals.transferInput.game = locals.game;
			locals.transferInput.destination = locals.game.owner;
			locals.transferInput.amount = locals.game.pendingCreatorCurrencyPayout;
			CALL(TransferGameCurrency, locals.transferInput, locals.transferOutput);
			if (locals.transferOutput.transferResult < 0)
			{
				output.returnCode = EReturnCode::TRANSFER_FAILED;
				return;
			}
			locals.game.pendingCreatorCurrencyPayout = 0;
			state.mut().games.set(input.slot, locals.game);
		}
		if (locals.game.pendingCreatorBalancePayout > 0)
		{
			locals.transferInput.game = locals.game;
			locals.transferInput.destination = locals.game.owner;
			locals.transferInput.amount = locals.game.pendingCreatorBalancePayout;
			CALL(TransferGameCurrency, locals.transferInput, locals.transferOutput);
			if (locals.transferOutput.transferResult < 0)
			{
				output.returnCode = EReturnCode::TRANSFER_FAILED;
				return;
			}
			locals.game.pendingCreatorBalancePayout = 0;
			state.mut().games.set(input.slot, locals.game);
		}
		if (locals.game.pendingRunCreditPayout > 0)
		{
			locals.transferResult = qpi.transfer(locals.game.owner, static_cast<sint64>(locals.game.pendingRunCreditPayout));
			if (locals.transferResult < 0)
			{
				output.returnCode = EReturnCode::TRANSFER_FAILED;
				return;
			}
			locals.game.pendingRunCreditPayout = 0;
			state.mut().games.set(input.slot, locals.game);
		}
		if (locals.game.finalizingStopReason == EGameStopReason::NONE)
		{
			locals.developer1Fee = mulDiv(locals.game.roundFeeSnapshot, PLDT_PLATFORM_DEV1_SHARE_PERCENT, 100ULL);
			locals.developer2Fee = mulDiv(locals.game.roundFeeSnapshot, PLDT_PLATFORM_DEV2_SHARE_PERCENT, 100ULL);
			locals.dividendFee = locals.game.roundFeeSnapshot - locals.developer1Fee - locals.developer2Fee;
			if (state.get().developer1Accrued > PLDT_MAX_TRANSFER_AMOUNT - locals.developer1Fee ||
			    state.get().developer2Accrued > PLDT_MAX_TRANSFER_AMOUNT - locals.developer2Fee ||
			    state.get().dividendAccrued > PLDT_MAX_TRANSFER_AMOUNT - locals.dividendFee)
			{
				output.returnCode = EReturnCode::STORAGE_FULL;
				return;
			}
		}
		// Publish the completed round before either clearing the game or committing its successor.
		setMemory(locals.result, 0);
		locals.result.tierWeightsBps = locals.game.tierWeightsBps;
		locals.result.winningDigits = locals.progress.winningDigits;
		locals.result.currencyAsset = locals.game.currencyAsset;
		locals.result.owner = locals.game.owner;
		locals.result.startAt = locals.game.startAt;
		locals.result.drawAt = locals.game.drawAt;
		locals.result.gameId = locals.game.gameId;
		locals.result.roundNumber = locals.game.roundNumber;
		locals.result.resultSequence = state.get().resultCounter + 1;
		locals.result.prizePool = locals.game.prizePool;
		locals.result.totalPaid = locals.game.totalPaid;
		locals.result.firstTicketLink = locals.game.firstTicketLink;
		locals.result.settledTick = qpi.tick();
		locals.result.ticketCount = locals.game.ticketCount;
		locals.result.winnerCount = locals.game.winnerCount;
		locals.result.codeLength = locals.game.codeLength;
		locals.result.currencyMode = locals.game.currencyMode;
		locals.result.mode = locals.game.mode;
		locals.result.terminalReason = locals.game.finalizingRoundReason;
		locals.result.detailsAvailable = true;
		locals.result.gameStopReason = locals.game.finalizingStopReason;
		if (locals.result.gameStopReason != EGameStopReason::NONE)
		{
			locals.resultIndex = mod(state.get().resultCounter, static_cast<uint64>(PLDT_RESULT_STORAGE_SIZE));
			state.mut().results.set(locals.resultIndex, locals.result);
			state.mut().resultCounter = state.get().resultCounter + 1;
			locals.clearInput.slot = input.slot;
			CALL(ClearGameSlot, locals.clearInput, locals.clearOutput);
			output.returnCode = EReturnCode::SUCCESS;
			return;
		}
		locals.resultIndex = mod(state.get().resultCounter, static_cast<uint64>(PLDT_RESULT_STORAGE_SIZE));
		state.mut().results.set(locals.resultIndex, locals.result);
		state.mut().resultCounter = state.get().resultCounter + 1;
		// Charge and initialize the next round only after result publication and all terminal transfers succeed.
		locals.game.runCredit -= locals.game.roundFeeSnapshot;
		locals.game.creatorBalance -= locals.game.creatorPrizeSeed;
		locals.developer1Fee = mulDiv(locals.game.roundFeeSnapshot, PLDT_PLATFORM_DEV1_SHARE_PERCENT, 100ULL);
		locals.developer2Fee = mulDiv(locals.game.roundFeeSnapshot, PLDT_PLATFORM_DEV2_SHARE_PERCENT, 100ULL);
		state.mut().developer1Accrued = sadd(state.get().developer1Accrued, locals.developer1Fee);
		state.mut().developer2Accrued = sadd(state.get().developer2Accrued, locals.developer2Fee);
		state.mut().dividendAccrued = sadd(state.get().dividendAccrued, locals.game.roundFeeSnapshot - locals.developer1Fee - locals.developer2Fee);
		++locals.game.roundNumber;
		locals.game.startAt = locals.nextStartAt;
		locals.game.drawAt = locals.nextDrawAt;
		locals.game.prizePool = locals.game.creatorPrizeSeed;
		locals.game.totalRevenue = 0;
		locals.game.totalPaid = 0;
		locals.game.firstTicketLink = 0;
		locals.game.lastTicketLink = 0;
		locals.game.ticketCount = 0;
		locals.game.winnerCount = 0;
		locals.game.status = EGameStatus::SELLING;
		setMemory(locals.progress, 0);
		state.mut().settlements.set(input.slot, locals.progress);
		state.mut().games.set(input.slot, locals.game);
		output.returnCode = EReturnCode::SUCCESS;
	}

	PRIVATE_PROCEDURE_WITH_LOCALS(ClearGameSlot)
	{
		locals.game = state.get().games.get(input.slot);
		if (locals.game.currencyMode == ECurrencyMode::ASSET && locals.game.assetAccountingLink > 0)
		{
			locals.assetAccounting = state.get().assetAccounting.get(locals.game.assetAccountingLink - 1);
			if (locals.assetAccounting.activeGameCount > 0)
			{
				--locals.assetAccounting.activeGameCount;
			}
			if (locals.assetAccounting.activeGameCount == 0 && locals.assetAccounting.developer1Accrued == 0 &&
			    locals.assetAccounting.developer2Accrued == 0 && locals.assetAccounting.dividendAccrued == 0)
			{
				setMemory(locals.assetAccounting, 0);
			}
			state.mut().assetAccounting.set(locals.game.assetAccountingLink - 1, locals.assetAccounting);
		}
		setMemory(locals.emptyGame, 0);
		setMemory(locals.emptyProgress, 0);
		state.mut().games.set(input.slot, locals.emptyGame);
		state.mut().settlements.set(input.slot, locals.emptyProgress);
		if (state.get().activeGameCount > 0)
		{
			state.mut().activeGameCount = state.get().activeGameCount - 1;
		}
		state.mut().allocationCursor = input.slot;
	}

	PRIVATE_PROCEDURE_WITH_LOCALS(ProcessGame)
	{
		output.actionsUsed = 0;
		locals.game = state.get().games.get(input.slot);
		if (locals.game.status == EGameStatus::FINALIZING)
		{
			locals.finalizeInput.slot = input.slot;
			locals.finalizeInput.reason = locals.game.finalizingRoundReason;
			CALL(FinalizeGame, locals.finalizeInput, locals.finalizeOutput);
			output.returnCode = locals.finalizeOutput.returnCode;
			return;
		}
		locals.now = qpi.now();
		locals.lifecycleInput.game = locals.game;
		locals.lifecycleInput.now = locals.now;
		CALL(EvaluateGameLifecycle, locals.lifecycleInput, locals.lifecycleOutput);
		if (locals.lifecycleOutput.action == EGameLifecycleAction::OPEN_SALES)
		{
			locals.game.status = locals.lifecycleOutput.effectiveStatus;
			state.mut().games.set(input.slot, locals.game);
		}
		if (locals.lifecycleOutput.action == EGameLifecycleAction::FINALIZE_NO_TICKETS)
		{
			locals.finalizeInput.slot = input.slot;
			locals.finalizeInput.reason = EGameTerminalReason::NO_TICKETS;
			CALL(FinalizeGame, locals.finalizeInput, locals.finalizeOutput);
			output.returnCode = locals.finalizeOutput.returnCode;
			return;
		}
		if (locals.lifecycleOutput.action == EGameLifecycleAction::BEGIN_SETTLEMENT)
		{
			locals.game.status = EGameStatus::CLOSED;
			state.mut().games.set(input.slot, locals.game);
			locals.beginInput.slot = input.slot;
			CALL(BeginSettlement, locals.beginInput, locals.beginOutput);
		}
		if (input.actionBudget > 0 &&
		    (state.get().games.get(input.slot).status == EGameStatus::COUNTING || state.get().games.get(input.slot).status == EGameStatus::PAYING))
		{
			locals.advanceInput.slot = input.slot;
			locals.advanceInput.actionBudget = input.actionBudget;
			CALL(AdvanceSettlement, locals.advanceInput, locals.advanceOutput);
			output.actionsUsed = locals.advanceOutput.actionsUsed;
			output.returnCode = locals.advanceOutput.returnCode;
			return;
		}
		output.returnCode = EReturnCode::SUCCESS;
	}

	PRIVATE_PROCEDURE_WITH_LOCALS(ReclaimCompletedTickets)
	{
		// Preserve the guaranteed history window unless ticket pressure requires reclaiming older details.
		output.actionsUsed = 0;
		if (!state.get().reclaimingTickets &&
		    (state.get().freeTicketCount > 0 || state.get().nextUnusedTicketSlot < state.get().tickets.capacity()) &&
		    state.get().resultCounter - state.get().reclaimResultCounter < PLDT_RESULT_HISTORY_SIZE)
		{
			return;
		}
		locals.budget = input.actionBudget;
		locals.resultScans = 0;
		// Mark a result's details unavailable before recycling its ticket chain, preserving lookup consistency.
		while (locals.budget > 0 && locals.resultScans < PLDT_SETTLEMENT_ACTION_BUDGET)
		{
			while (!state.get().reclaimingTickets && state.get().reclaimResultCounter < state.get().resultCounter &&
			       locals.resultScans < PLDT_SETTLEMENT_ACTION_BUDGET)
			{
				locals.resultIndex = mod(state.get().reclaimResultCounter, static_cast<uint64>(PLDT_RESULT_STORAGE_SIZE));
				locals.result = state.get().results.get(static_cast<uint16>(locals.resultIndex));
				++locals.resultScans;
				if (locals.result.resultSequence == state.get().reclaimResultCounter + 1 && locals.result.detailsAvailable &&
				    locals.result.firstTicketLink != 0)
				{
					locals.result.detailsAvailable = false;
					state.mut().results.set(static_cast<uint16>(locals.resultIndex), locals.result);
					state.mut().reclaimTicketLink = locals.result.firstTicketLink;
					state.mut().reclaimingTickets = true;
					break;
				}
				state.mut().reclaimResultCounter = state.get().reclaimResultCounter + 1;
			}
			if (!state.get().reclaimingTickets)
			{
				break;
			}
			while (state.get().reclaimTicketLink != 0 && locals.budget > 0)
			{
				locals.ticketSlot = state.get().reclaimTicketLink - 1;
				locals.ticket = state.get().tickets.get(locals.ticketSlot);
				locals.nextLink = locals.ticket.nextLink;
				locals.ticket.nextLink = state.get().freeTicketHead;
				state.mut().tickets.set(locals.ticketSlot, locals.ticket);
				state.mut().freeTicketHead = locals.ticketSlot + 1;
				state.mut().freeTicketCount = state.get().freeTicketCount + 1;
				state.mut().reclaimTicketLink = locals.nextLink;
				--locals.budget;
			}
			if (state.get().reclaimTicketLink == 0)
			{
				state.mut().reclaimingTickets = false;
				state.mut().reclaimResultCounter = state.get().reclaimResultCounter + 1;
			}
		}
		output.actionsUsed = input.actionBudget - locals.budget;
	}

	/** Extracts the fixed game slot encoded in a generation-aware game id. */
	static constexpr uint16 gameSlot(const uint64 gameId) { return static_cast<uint16>(gameId & (PLDT_MAX_GAMES - 1)); }

	/** Verifies that a game id still names the active generation occupying its encoded slot. */
	static bool isGameIdValid(const QPI::ContractState<StateData, CONTRACT_INDEX>& state, const uint64 gameId)
	{
		return gameId > 0 && state.get().games.get(gameSlot(gameId)).status != EGameStatus::EMPTY_SLOT &&
		       state.get().games.get(gameSlot(gameId)).gameId == gameId;
	}

	/** Computes floor(value * multiplier / divisor) without overflowing the intermediate product. */
	static uint64 mulDiv(const uint64 value, const uint64 multiplier, const uint64 divisor)
	{
		return sadd(smul(div(value, divisor), multiplier), div(smul(mod(value, divisor), multiplier), divisor));
	}

	/** Computes (value * multiplier) mod divisor without overflowing the intermediate product. */
	static uint64 mulMod(const uint64 value, const uint64 multiplier, const uint64 divisor)
	{
		return mod(smul(mod(value, divisor), multiplier), divisor);
	}
};
