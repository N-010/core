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
/** Maximum number of unique player summaries returned by one query page. */
constexpr uint16 PLDT_PLAYERS_PAGE_CAPACITY = 64;
/** Collision-resolving lookup capacity for unique players in one round. */
constexpr uint16 PLDT_PLAYER_LOOKUP_CAPACITY = PLDT_MAX_TICKETS_PER_GAME * 2;
/** Maximum asset holdings that may qualify a player for the bonus multiplier. */
constexpr uint16 PLDT_MAX_BONUS_ASSETS = 8;
/** Maximum count of digits in a submitted or winning code. */
constexpr uint8 PLDT_MAX_CODE_LENGTH = 10;
/** Minimum code length that keeps the game space non-trivial. */
constexpr uint8 PLDT_MIN_CODE_LENGTH = 4;
/** Power-of-two digit-array capacity required by QPI Array. */
constexpr uint8 PLDT_DIGITS_ALIGNED = 16;
/** Largest digit value supported by the fixed uniqueness workspace. */
constexpr uint8 PLDT_MAX_DIGIT = PLDT_MAX_CODE_LENGTH - 1;
/** Minimum inclusive maximum digit accepted by a game. */
constexpr uint8 PLDT_MIN_MAX_DIGIT = 7;
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
/** Default restricted service credit required to open a creator wallet. */
constexpr uint64 PLDT_DEFAULT_WALLET_CREATION_FEE = 1000000;
/** Fee burned by creator game-management mutations after authorization. */
constexpr uint64 PLDT_OPERATION_FEE = 100;
/** Maximum number of creator wallets retained in contract state. */
constexpr uint16 PLDT_MAX_WALLETS = 1024;
/** Internal power-of-two wallet map capacity kept below its high-load range. */
constexpr uint16 PLDT_WALLET_MAP_CAPACITY = 2048;
/** Maximum distinct managed assets tracked by one creator wallet. */
constexpr uint8 PLDT_MAX_WALLET_ASSETS = 16;
/** Power-of-two capacity covering every possible PLDT shareholder recipient. */
constexpr uint16 PLDT_MAX_DIVIDEND_RECIPIENTS = 1024;
/** Tick interval between automated lifecycle scans. */
constexpr uint32 PLDT_TICK_UPDATE_PERIOD = 100;
/** Number of game slots inspected during one automation pass. */
constexpr uint16 PLDT_AUTOMATION_GAMES_PER_TICK = 32;
/** Number of wallet-map slots inspected during one periodic automation pass. */
constexpr uint16 PLDT_AUTOMATION_WALLETS_PER_TICK = 32;
/** Maximum managed-asset positions released or detached per wallet automation pass. */
constexpr uint8 PLDT_WALLET_EXPIRY_ASSET_ACTION_BUDGET = 16;
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

	/** Lifecycle state of a creator wallet. */
	enum class EWalletStatus : uint8
	{
		/** Zero state used only for absent records. */
		ABSENT,
		/** Wallet accepts deposits and may fund games. */
		OPEN,
		/** Wallet is being removed by bounded background processing. */
		EXPIRING,
	};

	/** Determines whether a game ends once or schedules successive rounds. */
	enum class EGameMode : uint8
	{
		/** A single funded round that clears its slot after finalization. */
		ONE_SHOT,
		/** A creator-funded game that may schedule successive rounds. */
		PERMANENT,
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
		/** Creator fees awaiting resumable credit to the internal wallet. */
		uint64 pendingCreatorCurrencyPayout;
		/** Returned pool and unused creator balance awaiting internal wallet credit. */
		uint64 pendingCreatorBalancePayout;
		/** Unused Qubic run credit awaiting internal wallet credit with preserved provenance. */
		uint64 pendingRunCreditPayout;
		/** Service credit provenance still contained exclusively in the run-credit ledger. */
		uint64 runServiceCredit;
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

	/** One asset balance tracked inside a creator wallet. */
	struct WalletAssetBalance
	{
		/** Asset whose management rights are held by PulseEditor. */
		Asset asset;
		/** Shares available to fund games or transfer management rights. */
		uint64 balance;
		/** Active games that keep this asset position addressable at zero balance. */
		uint16 activeGameReferences;
		/** Whether this fixed-capacity position is allocated. */
		bit isActive;
	};

	/** Creator-owned funding ledger used by all game-creation operations. */
	struct CreatorWallet
	{
		/** Fixed-capacity managed-asset ledger. */
		Array<WalletAssetBalance, PLDT_MAX_WALLET_ASSETS> assets;
		/** Restricted QU usable for operation fees/run credit and withdrawable after the last game closes. */
		uint64 serviceCredit;
		/** QU that may fund games or be withdrawn by the wallet owner. */
		uint64 refundableQubic;
		/** Number of game slots currently owned by this wallet. */
		uint16 activeGameCount;
		/** Epoch of the most recent successful game creation. */
		uint16 lastGameCreationEpoch;
		/** Number of allocated entries in assets. */
		uint8 assetCount;
		/** Next asset position considered during bounded expiry. */
		uint8 expiryAssetCursor;
		/** Current wallet lifecycle state. */
		EWalletStatus status;
		/** Whether service credit became withdrawable after the wallet's last game closed. */
		bit serviceCreditUnlocked;
	};

	/** Snapshot and retry cursor for one in-flight managed-asset dividend. */
	struct AssetDividendDistribution
	{
		/** Shareholders captured before the first external transfer. */
		Array<id, PLDT_MAX_DIVIDEND_RECIPIENTS> recipients;
		/** Immutable entitlement paired with each captured shareholder. */
		Array<uint64, PLDT_MAX_DIVIDEND_RECIPIENTS> amounts;
		/** Asset whose custody is being distributed. */
		Asset asset;
		/** Amount from the snapshot still owed to recipients. */
		uint64 remainingAmount;
		/** Asset-accounting slot that owns this liability. */
		uint16 accountingSlot;
		/** Number of populated snapshot entries. */
		uint16 recipientCount;
		/** First unpaid snapshot entry. */
		uint16 cursor;
		/** Whether the snapshot must be resumed before another asset distribution. */
		bit active;
	};

	/** All consensus-persistent PulseEditor state; field order is ABI-sensitive. */
	struct StateData
	{
		/** Creator funding ledgers indexed by creator identity. */
		HashMap<id, CreatorWallet, PLDT_WALLET_MAP_CAPACITY> wallets;
		/** Generation-aware game records indexed by game slot. */
		Array<Game, PLDT_MAX_GAMES> games;
		/** Per-game resumable settlement progress. */
		Array<SettlementProgress, PLDT_MAX_GAMES> settlements;
		/** Per-slot generation counters used to create fresh game ids. */
		Array<uint64, PLDT_MAX_GAMES> generations;
		/** One-based links connecting cleared game slots for constant-time reuse. */
		Array<uint16, PLDT_MAX_GAMES> freeGameNext;
		/** Generation-aware ticket records shared by all games. */
		Array<Ticket, PLDT_MAX_TICKETS> tickets;
		/** Bounded ring of published round results. */
		Array<GameResult, PLDT_RESULT_STORAGE_SIZE> results;
		/** Working copy of the game currency asset's accrual bucket. */
		Array<AssetPlatformAccounting, PLDT_MAX_GAMES> assetAccounting;
		/** Single resumable dividend snapshot; platform withdrawals are serialized around it. */
		AssetDividendDistribution assetDividendDistribution;
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
		/** Service credit retained when a new creator wallet is opened. */
		uint64 walletCreationFee;
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
		/** One-based head of the reusable game-slot free list. */
		uint16 freeGameHead;
		/** First game slot that has never been allocated. */
		uint16 nextUnusedGameSlot;
		/** Next game slot from which the periodic scan resumes. */
		uint16 automationCursor;
		/** Next raw wallet-map slot from which the periodic scan resumes. */
		uint16 walletAutomationCursor;
		/** Ticket-price percentage reserved for platform recipients. */
		uint8 platformFeePercent;
		/** Owner-configurable upper bound for creatorFeePercent. */
		uint8 maxCreatorFeePercent;
		/** Whether background cleanup is traversing this result's ticket chain. */
		bit reclaimingTickets;
	};

	/** Empty request used to create the invocator's creator wallet. */
	struct CreateWallet_input
	{
	};

	/** Result of creating a creator wallet. */
	struct CreateWallet_output
	{
		/** Restricted service credit assigned to the wallet. */
		uint64 serviceCredit;
		/** Excess invocation reward retained as withdrawable wallet QU. */
		uint64 refundableQubic;
		/** Public outcome describing success or why the wallet was not created. */
		EReturnCode returnCode;
	};

	/** Identifies the creator wallet to read. */
	struct GetWallet_input
	{
		/** Wallet owner whose ledger is requested. */
		id owner;
	};

	/** Public snapshot of a creator wallet. */
	struct GetWallet_output
	{
		/** Managed-asset positions, including positions retained by active games. */
		Array<WalletAssetBalance, PLDT_MAX_WALLET_ASSETS> assets;
		/** Restricted QU available for operation fees and run credit. */
		uint64 serviceCredit;
		/** Withdrawable QU available for game funding. */
		uint64 refundableQubic;
		/** Number of game slots currently owned by this wallet. */
		uint16 activeGameCount;
		/** Epoch of the latest successful game creation. */
		uint16 lastGameCreationEpoch;
		/** Number of allocated asset positions. */
		uint8 assetCount;
		/** Current wallet lifecycle state. */
		EWalletStatus status;
		/** Whether the requested owner has a wallet. */
		bit found;
		/** Whether service credit may be withdrawn while no game is active. */
		bit serviceCreditUnlocked;
	};

	/** Empty request used to deposit the invocation reward into a wallet. */
	struct DepositWalletQubic_input
	{
	};

	/** Result of a refundable-Qubic wallet deposit. */
	struct DepositWalletQubic_output
	{
		/** Updated refundable balance after a successful deposit. */
		uint64 refundableQubic;
		/** Public outcome describing success or why no deposit was retained. */
		EReturnCode returnCode;
	};

	/** Requests refundable Qubic from the invocator's wallet. */
	struct WithdrawWalletQubic_input
	{
		/** Amount of refundable Qubic to return to the wallet owner. */
		uint64 amount;
	};

	/** Result of withdrawing refundable wallet Qubic. */
	struct WithdrawWalletQubic_output
	{
		/** Amount transferred successfully. */
		uint64 amountPaid;
		/** Public outcome describing success or why no withdrawal occurred. */
		EReturnCode returnCode;
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
		/** Wallet fee burned by each authorized creator mutation. */
		uint64 operationFee;
		/** Wallet Qubic required for initial run credit and Qubic creator funding. */
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
		/** Service credit required to open a new creator wallet. */
		uint64 walletCreationFee;
		/** Number of tickets accepted for the current request or round. */
		uint64 ticketCount;
		/** Monotonic sequence used to order and place published results. */
		uint64 resultCounter;
		/** Total game slots currently occupied. */
		uint16 activeGameCount;
		/** Number of creator wallets currently retained. */
		uint16 walletCount;
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
		/** Service credit required to open creator wallets after this update. */
		uint64 walletCreationFee;
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

	/** Aggregated ticket and payout data for one unique player in a round. */
	struct PlayerSummary
	{
		/** Player identity, ordered by this player's first ticket in the round. */
		id player;
		/** Sum of payout values across all of the player's tickets in the round. */
		uint64 totalPayout;
		/** Total number of tickets owned by the player in the round. */
		uint16 ticketCount;
		/** Number of tickets whose settlement winner weight is non-zero. */
		uint16 winningTicketCount;
		/** Number of tickets whose persisted status is PAID. */
		uint16 paidTicketCount;
		/** Number of tickets that qualified for the bonus multiplier. */
		uint16 bonusQualifiedTicketCount;
	};

	/** Validated data consumed by the unique-player paging operation. */
	struct GetPlayers_input
	{
		/** Game and round pair whose unique players are requested. */
		RoundKey roundKey;
		/** Zero-based number of unique players to skip. */
		uint64 offset;
		/** Maximum requested page size; values above 64 are clamped to 64. */
		uint16 limit;
		/** Reserved ABI bytes; callers must initialize them to zero. */
		Array<uint8, 4> padding0;
		/** Reserved ABI bytes; callers must initialize them to zero. */
		Array<uint8, 2> padding1;
	};

	/** Result data produced by the unique-player paging operation. */
	struct GetPlayers_output
	{
		/** Unique player summaries in stable first-ticket order. */
		Array<PlayerSummary, PLDT_PLAYERS_PAGE_CAPACITY> players;
		/** Complete unique-player count, independent of offset and limit. */
		uint64 totalCount;
		/** Number of valid summaries written into the current page. */
		uint16 returnedCount;
		/** Public outcome describing success or the lookup failure. */
		EReturnCode returnCode;
		/** Reserved ABI bytes, always returned as zero. */
		Array<uint8, 4> padding0;
		/** Reserved ABI byte, always returned as zero. */
		Array<uint8, 1> padding1;
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

	/** Validated data consumed by the wallet asset credit operation. */
	struct CreditWalletAsset_input
	{
		/** Wallet owner receiving managed asset shares. */
		id owner;
		/** Managed asset whose wallet balance may be credited. */
		Asset asset;
		/** Number of transferred shares to credit. */
		uint64 amount;
	};

	/** Result data produced by the wallet asset credit operation. */
	struct CreditWalletAsset_output
	{
		/** Whether the transferred shares were represented in the destination wallet. */
		bit credited;
	};

	/** Validated data consumed by the transfer asset dividend operation. */
	struct TransferAssetDividend_input
	{
		/** Asset issuance distributed proportionally to PLDT shareholders. */
		Asset dividendAsset;
		/** Asset units available for proportional shareholder distribution. */
		uint64 dividendAmount;
		/** Asset-accounting slot that owns the dividend liability. */
		uint16 accountingSlot;
	};

	/** Result data produced by the transfer asset dividend operation. */
	struct TransferAssetDividend_output
	{
		/** Asset units already transferred during proportional shareholder distribution. */
		uint64 distributedAmount;
		/** Whether any resumable transfer attempted in this operation failed. */
		bit failed;
	};

	/** Input for freezing one immutable managed-asset dividend entitlement set. */
	using SnapshotAssetDividend_input = TransferAssetDividend_input;

	/** Result of freezing a managed-asset dividend entitlement set. */
	struct SnapshotAssetDividend_output
	{
		/** Whether the shareholder set could not be represented completely. */
		bit failed;
	};

	/** QPI scratch state isolated from payout locals to stay within the locals-size limit. */
	struct SnapshotAssetDividend_locals
	{
		/** Iterator state for the current PLDT shareholder set. */
		AssetPossessionIterator shareholdersIter;
		/** Contract-share asset whose holders receive the dividend. */
		Asset shareholdersAsset;
		/** Whole currency units still unallocated after integer division. */
		uint64 remainder;
		/** Remainder assigned to the current holder. */
		uint64 holderRemainder;
		/** Whole asset units assigned per held PLDT share. */
		sint64 dividendPerShare;
		/** PLDT shares possessed by the current shareholder. */
		sint64 holderShares;
		/** Asset units assigned to the current shareholder. */
		sint64 holderDividend;
	};

	/** QPI scratch state for transfer asset dividend; contract routines cannot declare stack locals. */
	struct TransferAssetDividend_locals
	{
		/** Request used to mirror a successful shareholder payout when possible. */
		CreditWalletAsset_input creditInput;
		/** Result of the best-effort wallet mirror. */
		CreditWalletAsset_output creditOutput;
		/** Request used to freeze dividend entitlements before transfers begin. */
		SnapshotAssetDividend_input snapshotInput;
		/** Result of freezing dividend entitlements. */
		SnapshotAssetDividend_output snapshotOutput;
		/** QPI transfer result; a negative value indicates failure. */
		sint64 transferResult;
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

	/** QPI scratch state for withdraw asset platform revenue; contract routines cannot declare stack locals. */
	struct WithdrawAssetPlatformRevenue_locals
	{
		/** Request used to mirror a successful developer payout when possible. */
		CreditWalletAsset_input creditInput;
		/** Result of the best-effort wallet mirror. */
		CreditWalletAsset_output creditOutput;
		/** Request passed to the dividend helper. */
		TransferAssetDividend_input dividendInput;
		/** Response returned by the dividend helper. */
		TransferAssetDividend_output dividendOutput;
		/** Working copy of one asset platform-accrual bucket. */
		AssetPlatformAccounting accounting;
		/** Request passed to the refund helper. */
		RefundInvocationReward_input refundInput;
		/** Response returned by the refund helper. */
		RefundInvocationReward_output refundOutput;
		/** Loop cursor for the bounded collection being processed. */
		uint64 i;
		/** QPI transfer result; a negative value indicates failure. */
		sint64 transferResult;
		/** Whether the requested record was found during bounded lookup. */
		bit found;
		/** Whether any resumable transfer attempted in this operation failed. */
		bit failed;
	};

	/** QPI scratch state for releasing creator-managed asset rights. */
	struct TransferShareManagementRights_locals
	{
		/** Creator wallet snapshot used while releasing management rights. */
		CreatorWallet wallet;
		/** Scratch managed-asset position used during the operation. */
		WalletAssetBalance walletAsset;
		/** Scratch bounded-loop index used during the operation. */
		uint64 i;
		/** Scratch refundable amount used during the operation. */
		sint64 refundAmount;
		/** Scratch refund result used during the operation. */
		sint64 refundResult;
		/** Managed shares currently possessed by the caller under this contract. */
		sint64 possessedShares;
		/** Tracked wallet shares removed after a successful release. */
		uint64 walletDebit;
		/** Scratch wallet asset slot used during the operation. */
		uint8 walletAssetSlot;
		/** Scratch wallet lookup-success flag used during the operation. */
		bit walletFound;
		/** Scratch wallet asset lookup-success flag used during the operation. */
		bit walletAssetFound;
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

	/** Input values consumed by the validate game configuration helper. */
	struct ValidateGameConfiguration_input
	{
		/** Configuration stored by this structure. */
		PreviewGame_input configuration;
	};

	/** Result values produced by the validate game configuration helper. */
	struct ValidateGameConfiguration_output
	{
		/** Operation result code stored by this structure. */
		EReturnCode returnCode;
	};

	/** QPI scratch state used by the validate game configuration helper. */
	struct ValidateGameConfiguration_locals
	{
		/** Max draw at stored by this structure. */
		DateAndTime maxDrawAt;
		/** Bounded-loop index stored by this structure. */
		uint64 i;
		/** Managed currency shares stored by this structure. */
		sint64 managedCurrencyShares;
	};

	/** Input values consumed by the validate tier configuration helper. */
	struct ValidateTierConfiguration_input
	{
		/** Tier weights bps stored by this structure. */
		TierWeightMatrix tierWeightsBps;
		/** Code length stored by this structure. */
		uint8 codeLength;
	};

	/** Result values produced by the validate tier configuration helper. */
	struct ValidateTierConfiguration_output
	{
		/** Operation result code stored by this structure. */
		EReturnCode returnCode;
	};

	/** QPI scratch state used by the validate tier configuration helper. */
	struct ValidateTierConfiguration_locals
	{
		/** Weight total stored by this structure. */
		uint64 weightTotal;
		/** Index stored by this structure. */
		uint64 index;
		/** Exact stored by this structure. */
		uint8 exact;
		/** Misplaced stored by this structure. */
		uint8 misplaced;
	};

	/** Input values consumed by the calculate game preview helper. */
	struct CalculateGamePreview_input
	{
		/** Configuration stored by this structure. */
		PreviewGame_input configuration;
	};

	/** Result values produced by the calculate game preview helper. */
	struct CalculateGamePreview_output
	{
		/** Preview stored by this structure. */
		PreviewGame_output preview;
	};

	/** QPI scratch state used by the calculate game preview helper. */
	struct CalculateGamePreview_locals
	{
		/** Economics input stored by this structure. */
		CalculateTicketEconomics_input economicsInput;
		/** Economics output stored by this structure. */
		CalculateTicketEconomics_output economicsOutput;
		/** Refundable per ticket stored by this structure. */
		uint64 refundablePerTicket;
	};

	/** QPI scratch state for preview game; contract routines cannot declare stack locals. */
	struct PreviewGame_locals
	{
		/** Configuration input stored by this structure. */
		ValidateGameConfiguration_input configurationInput;
		/** Configuration output stored by this structure. */
		ValidateGameConfiguration_output configurationOutput;
		/** Tier input stored by this structure. */
		ValidateTierConfiguration_input tierInput;
		/** Tier output stored by this structure. */
		ValidateTierConfiguration_output tierOutput;
		/** Preview input stored by this structure. */
		CalculateGamePreview_input previewInput;
		/** Preview output stored by this structure. */
		CalculateGamePreview_output previewOutput;
	};

	/** QPI scratch state used by the wallet asset credit operation. */
	struct CreditWalletAsset_locals
	{
		/** Working copy of the destination wallet. */
		CreatorWallet wallet;
		/** Working copy of one wallet asset position. */
		WalletAssetBalance walletAsset;
		/** Bounded-loop index used to inspect wallet asset slots. */
		uint64 i;
		/** First inactive asset slot available for a new position. */
		uint8 freeSlot;
		/** Whether the asset already has a wallet position. */
		bit found;
		/** Whether an inactive asset slot is available. */
		bit freeSlotFound;
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

	/** Input values consumed by the prepare game creation helper. */
	struct PrepareGameCreation_input
	{
		/** Configuration stored by this structure. */
		CreateGame_input configuration;
		/** Preview stored by this structure. */
		PreviewGame_output preview;
	};

	/** Result values produced by the prepare game creation helper. */
	struct PrepareGameCreation_output
	{
		/** Asset accounting stored by this structure. */
		AssetPlatformAccounting assetAccounting;
		/** Developer1 fee stored by this structure. */
		uint64 developer1Fee;
		/** Developer2 fee stored by this structure. */
		uint64 developer2Fee;
		/** Dividend fee stored by this structure. */
		uint64 dividendFee;
		/** Service credit debit stored by this structure. */
		uint64 serviceCreditDebit;
		/** Refundable qubic debit stored by this structure. */
		uint64 refundableQubicDebit;
		/** Bounded storage slot stored by this structure. */
		uint16 slot;
		/** Accounting slot stored by this structure. */
		uint16 accountingSlot;
		/** Wallet asset slot stored by this structure. */
		uint8 walletAssetSlot;
		/** Operation result code stored by this structure. */
		EReturnCode returnCode;
	};

	/** QPI scratch state used by the prepare game creation helper. */
	struct PrepareGameCreation_locals
	{
		/** Asset accounting stored by this structure. */
		AssetPlatformAccounting assetAccounting;
		/** Wallet stored by this structure. */
		CreatorWallet wallet;
		/** Wallet asset stored by this structure. */
		WalletAssetBalance walletAsset;
		/** Game stored by this structure. */
		Game game;
		/** Max draw at stored by this structure. */
		DateAndTime maxDrawAt;
		/** Bounded-loop index stored by this structure. */
		uint64 i;
		/** Candidate slot stored by this structure. */
		uint16 candidateSlot;
		/** Creator active games stored by this structure. */
		uint16 creatorActiveGames;
		/** Required qubic stored by this structure. */
		uint64 requiredQubic;
		/** Wallet free asset slot stored by this structure. */
		uint8 walletFreeAssetSlot;
		/** Lookup-success flag stored by this structure. */
		bit found;
		/** Accounting found stored by this structure. */
		bit accountingFound;
		/** Wallet asset found stored by this structure. */
		bit walletAssetFound;
		/** Wallet free asset slot found stored by this structure. */
		bit walletFreeAssetSlotFound;
	};

	/** Input values consumed by the collect initial asset funding helper. */
	struct CollectInitialAssetFunding_input
	{
		/** Configuration stored by this structure. */
		CreateGame_input configuration;
	};

	/** Result values produced by the collect initial asset funding helper. */
	struct CollectInitialAssetFunding_output
	{
		/** Operation result code stored by this structure. */
		EReturnCode returnCode;
	};

	/** QPI scratch state used by the collect initial asset funding helper. */
	struct CollectInitialAssetFunding_locals
	{
		/** Possessed shares stored by this structure. */
		sint64 possessedShares;
		/** Transfer result stored by this structure. */
		sint64 transferResult;
	};

	/** Input values consumed by the commit created game helper. */
	struct CommitCreatedGame_input
	{
		/** Configuration stored by this structure. */
		CreateGame_input configuration;
		/** Prepared stored by this structure. */
		PrepareGameCreation_output prepared;
	};

	/** Result values produced by the commit created game helper. */
	struct CommitCreatedGame_output
	{
		/** Game id stored by this structure. */
		uint64 gameId;
		/** Bounded storage slot stored by this structure. */
		uint16 slot;
		/** Operation result code stored by this structure. */
		EReturnCode returnCode;
	};

	/** QPI scratch state used by the commit created game helper. */
	struct CommitCreatedGame_locals
	{
		/** Game stored by this structure. */
		Game game;
		/** Asset accounting stored by this structure. */
		AssetPlatformAccounting assetAccounting;
		/** Wallet stored by this structure. */
		CreatorWallet wallet;
		/** Wallet asset stored by this structure. */
		WalletAssetBalance walletAsset;
		/** Generation stored by this structure. */
		uint64 generation;
	};

	/** QPI scratch state for create game; contract routines cannot declare stack locals. */
	struct CreateGame_locals
	{
		/** Wallet debited by the creator-operation fee before validation. */
		CreatorWallet wallet;
		/** Refund input stored by this structure. */
		RefundInvocationReward_input refundInput;
		/** Refund output stored by this structure. */
		RefundInvocationReward_output refundOutput;
		/** Preview input stored by this structure. */
		PreviewGame_input previewInput;
		/** Preview output stored by this structure. */
		PreviewGame_output previewOutput;
		/** Prepare input stored by this structure. */
		PrepareGameCreation_input prepareInput;
		/** Prepare output stored by this structure. */
		PrepareGameCreation_output prepareOutput;
		/** Funding input stored by this structure. */
		CollectInitialAssetFunding_input fundingInput;
		/** Funding output stored by this structure. */
		CollectInitialAssetFunding_output fundingOutput;
		/** Commit input stored by this structure. */
		CommitCreatedGame_input commitInput;
		/** Commit output stored by this structure. */
		CommitCreatedGame_output commitOutput;
	};

	/** QPI scratch state for creator-wallet creation. */
	struct CreateWallet_locals
	{
		/** Wallet stored by this structure. */
		CreatorWallet wallet;
		/** Map index stored by this structure. */
		sint64 mapIndex;
		/** Transfer result stored by this structure. */
		sint64 transferResult;
	};

	/** QPI scratch state for reading a creator wallet. */
	struct GetWallet_locals
	{
		/** Wallet stored by this structure. */
		CreatorWallet wallet;
	};

	/** QPI scratch state for refundable-Qubic deposits. */
	struct DepositWalletQubic_locals
	{
		/** Wallet stored by this structure. */
		CreatorWallet wallet;
		/** Transfer result stored by this structure. */
		sint64 transferResult;
	};

	/** QPI scratch state for refundable-Qubic withdrawals. */
	struct WithdrawWalletQubic_locals
	{
		/** Wallet stored by this structure. */
		CreatorWallet wallet;
		/** Transfer result stored by this structure. */
		sint64 transferResult;
	};

	/** QPI scratch state for validating incoming share-management rights. */
	struct PRE_ACQUIRE_SHARES_locals
	{
		/** Wallet stored by this structure. */
		CreatorWallet wallet;
		/** Asset balance stored by this structure. */
		WalletAssetBalance assetBalance;
		/** Bounded-loop index stored by this structure. */
		uint64 i;
		/** Lookup-success flag stored by this structure. */
		bit found;
	};

	/** QPI scratch state for crediting incoming share-management rights. */
	struct POST_ACQUIRE_SHARES_locals
	{
		/** Request passed to the wallet asset credit helper. */
		CreditWalletAsset_input creditInput;
		/** Response returned by the wallet asset credit helper. */
		CreditWalletAsset_output creditOutput;
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
		/** Wallet stored by this structure. */
		CreatorWallet wallet;
		/** Wallet asset stored by this structure. */
		WalletAssetBalance walletAsset;
		/** Exact Qubic invocation reward required by the operation. */
		uint64 expectedReward;
		/** Service credit debit stored by this structure. */
		uint64 serviceCreditDebit;
		/** Bounded-loop index stored by this structure. */
		uint64 i;
		/** Managed asset shares currently possessed by the inspected entity. */
		sint64 possessedShares;
		/** QPI transfer result; a negative value indicates failure. */
		sint64 transferResult;
		/** Zero-based game, ticket, result, or accounting slot under operation. */
		uint16 slot;
		/** Wallet asset slot stored by this structure. */
		uint8 walletAssetSlot;
		/** Wallet asset found stored by this structure. */
		bit walletAssetFound;
	};

	/** QPI scratch state for withdraw game balance; contract routines cannot declare stack locals. */
	struct WithdrawGameBalance_locals
	{
		/** Working copy or returned snapshot of a game record. */
		Game game;
		/** Wallet stored by this structure. */
		CreatorWallet wallet;
		/** Wallet asset stored by this structure. */
		WalletAssetBalance walletAsset;
		/** Refund input stored by this structure. */
		RefundInvocationReward_input refundInput;
		/** Refund output stored by this structure. */
		RefundInvocationReward_output refundOutput;
		/** Qubic amount stored by this structure. */
		uint64 qubicAmount;
		/** Service credit returned stored by this structure. */
		uint64 serviceCreditReturned;
		/** Bounded-loop index stored by this structure. */
		uint64 i;
		/** QPI transfer result; a negative value indicates failure. */
		sint64 transferResult;
		/** Zero-based game, ticket, result, or accounting slot under operation. */
		uint16 slot;
		/** Whether any resumable transfer attempted in this operation failed. */
		bit failed;
		/** Wallet asset found stored by this structure. */
		bit walletAssetFound;
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
		/** Wallet debited by the creator-operation fee. */
		CreatorWallet wallet;
		/** Validated next-round economics before it is committed. */
		GameEconomics pending;
		/** Request passed to the refund helper. */
		RefundInvocationReward_input refundInput;
		/** Response returned by the refund helper. */
		RefundInvocationReward_output refundOutput;
		/** Creator plus prize amount that may need terminal return per ticket. */
		uint64 refundablePerTicket;
		/** Zero-based game, ticket, result, or accounting slot under operation. */
		uint16 slot;
	};

	/** Input values consumed by the execute ticket purchase helper. */
	struct ExecuteTicketPurchase_input
	{
		/** Purchase stored by this structure. */
		BuyTickets_input purchase;
		/** Validate digits before player limit stored by this structure. */
		bit validateDigitsBeforePlayerLimit;
	};

	/** Result values produced by the execute ticket purchase helper. */
	struct ExecuteTicketPurchase_output
	{
		/** Ticket ids stored by this structure. */
		Array<uint64, PLDT_MAX_BATCH_TICKETS> ticketIds;
		/** Ticket indexes stored by this structure. */
		Array<uint64, PLDT_MAX_BATCH_TICKETS> ticketIndexes;
		/** Prize contribution stored by this structure. */
		uint64 prizeContribution;
		/** Accepted count stored by this structure. */
		uint16 acceptedCount;
		/** Operation result code stored by this structure. */
		EReturnCode returnCode;
	};

	/** Input values consumed by the prepare ticket purchase helper. */
	struct PrepareTicketPurchase_input
	{
		/** Purchase stored by this structure. */
		BuyTickets_input purchase;
		/** Validate digits before player limit stored by this structure. */
		bit validateDigitsBeforePlayerLimit;
	};

	/** Result values produced by the prepare ticket purchase helper. */
	struct PrepareTicketPurchase_output
	{
		/** Economics stored by this structure. */
		CalculateTicketEconomics_output economics;
		/** Game stored by this structure. */
		Game game;
		/** Total price stored by this structure. */
		uint64 totalPrice;
		/** Bounded storage slot stored by this structure. */
		uint16 slot;
		/** Operation result code stored by this structure. */
		EReturnCode returnCode;
		/** Bonus qualified stored by this structure. */
		bit bonusQualified;
	};

	/** QPI scratch state used by the prepare ticket purchase helper. */
	struct PrepareTicketPurchase_locals
	{
		/** Lifecycle input stored by this structure. */
		EvaluateGameLifecycle_input lifecycleInput;
		/** Lifecycle output stored by this structure. */
		EvaluateGameLifecycle_output lifecycleOutput;
		/** Bonus input stored by this structure. */
		EvaluateBonusQualification_input bonusInput;
		/** Bonus output stored by this structure. */
		EvaluateBonusQualification_output bonusOutput;
		/** Economics input stored by this structure. */
		CalculateTicketEconomics_input economicsInput;
		/** Validate input stored by this structure. */
		ValidateDigits_input validateInput;
		/** Validate output stored by this structure. */
		ValidateDigits_output validateOutput;
		/** Ticket stored by this structure. */
		Ticket ticket;
		/** Now stored by this structure. */
		DateAndTime now;
		/** One-based storage link stored by this structure. */
		uint64 link;
		/** Player tickets stored by this structure. */
		uint64 playerTickets;
		/** Bounded-loop index stored by this structure. */
		uint64 i;
	};

	/** Input values consumed by the validate purchase accounting capacity helper. */
	struct ValidatePurchaseAccountingCapacity_input
	{
		/** Prepared stored by this structure. */
		PrepareTicketPurchase_output prepared;
		/** Ticket count stored by this structure. */
		uint16 ticketCount;
	};

	/** Result values produced by the validate purchase accounting capacity helper. */
	struct ValidatePurchaseAccountingCapacity_output
	{
		/** Operation result code stored by this structure. */
		EReturnCode returnCode;
		/** Refund on failure stored by this structure. */
		bit refundOnFailure;
	};

	/** QPI scratch state used by the validate purchase accounting capacity helper. */
	struct ValidatePurchaseAccountingCapacity_locals
	{
		/** Asset accounting stored by this structure. */
		AssetPlatformAccounting assetAccounting;
		/** Developer1 fee stored by this structure. */
		uint64 developer1Fee;
		/** Developer2 fee stored by this structure. */
		uint64 developer2Fee;
		/** Dividend fee stored by this structure. */
		uint64 dividendFee;
	};

	/** Input values consumed by the collect ticket payment helper. */
	struct CollectTicketPayment_input
	{
		/** Game stored by this structure. */
		Game game;
		/** Total price stored by this structure. */
		uint64 totalPrice;
		/** Ticket count stored by this structure. */
		uint16 ticketCount;
	};

	/** Result values produced by the collect ticket payment helper. */
	struct CollectTicketPayment_output
	{
		/** Operation result code stored by this structure. */
		EReturnCode returnCode;
	};

	/** QPI scratch state used by the collect ticket payment helper. */
	struct CollectTicketPayment_locals
	{
		/** Refund input stored by this structure. */
		RefundInvocationReward_input refundInput;
		/** Refund output stored by this structure. */
		RefundInvocationReward_output refundOutput;
		/** Transfer input stored by this structure. */
		TransferGameCurrency_input transferInput;
		/** Transfer output stored by this structure. */
		TransferGameCurrency_output transferOutput;
		/** Burn input stored by this structure. */
		BurnCollectedTicketPayment_input burnInput;
		/** Burn output stored by this structure. */
		BurnCollectedTicketPayment_output burnOutput;
		/** Wallet stored by this structure. */
		CreatorWallet wallet;
		/** Wallet asset stored by this structure. */
		WalletAssetBalance walletAsset;
		/** Wallet debit stored by this structure. */
		uint64 walletDebit;
		/** Bounded-loop index stored by this structure. */
		uint64 i;
		/** Possessed shares stored by this structure. */
		sint64 possessedShares;
		/** Transfer result stored by this structure. */
		sint64 transferResult;
		/** Wallet asset slot stored by this structure. */
		uint8 walletAssetSlot;
		/** Wallet found stored by this structure. */
		bit walletFound;
		/** Wallet asset found stored by this structure. */
		bit walletAssetFound;
	};

	/** Input values consumed by the commit ticket purchase helper. */
	struct CommitTicketPurchase_input
	{
		/** Purchase stored by this structure. */
		BuyTickets_input purchase;
		/** Prepared stored by this structure. */
		PrepareTicketPurchase_output prepared;
	};

	/** Result values produced by the commit ticket purchase helper. */
	struct CommitTicketPurchase_output
	{
		/** Ticket ids stored by this structure. */
		Array<uint64, PLDT_MAX_BATCH_TICKETS> ticketIds;
		/** Ticket indexes stored by this structure. */
		Array<uint64, PLDT_MAX_BATCH_TICKETS> ticketIndexes;
		/** Prize contribution stored by this structure. */
		uint64 prizeContribution;
		/** Accepted count stored by this structure. */
		uint16 acceptedCount;
		/** Operation result code stored by this structure. */
		EReturnCode returnCode;
	};

	/** QPI scratch state used by the commit ticket purchase helper. */
	struct CommitTicketPurchase_locals
	{
		/** Apply input stored by this structure. */
		ApplyAcceptedTicket_input applyInput;
		/** Apply output stored by this structure. */
		ApplyAcceptedTicket_output applyOutput;
		/** Bounded-loop index stored by this structure. */
		uint64 i;
	};

	/** QPI scratch state used by the execute ticket purchase helper. */
	struct ExecuteTicketPurchase_locals
	{
		/** Refund input stored by this structure. */
		RefundInvocationReward_input refundInput;
		/** Refund output stored by this structure. */
		RefundInvocationReward_output refundOutput;
		/** Prepare input stored by this structure. */
		PrepareTicketPurchase_input prepareInput;
		/** Prepare output stored by this structure. */
		PrepareTicketPurchase_output prepareOutput;
		/** Capacity input stored by this structure. */
		ValidatePurchaseAccountingCapacity_input capacityInput;
		/** Capacity output stored by this structure. */
		ValidatePurchaseAccountingCapacity_output capacityOutput;
		/** Payment input stored by this structure. */
		CollectTicketPayment_input paymentInput;
		/** Payment output stored by this structure. */
		CollectTicketPayment_output paymentOutput;
		/** Commit input stored by this structure. */
		CommitTicketPurchase_input commitInput;
		/** Commit output stored by this structure. */
		CommitTicketPurchase_output commitOutput;
	};

	/** QPI scratch state used by the buy ticket helper. */
	struct BuyTicket_locals
	{
		/** Purchase input stored by this structure. */
		ExecuteTicketPurchase_input purchaseInput;
		/** Purchase output stored by this structure. */
		ExecuteTicketPurchase_output purchaseOutput;
	};

	/** QPI scratch state used by the buy tickets helper. */
	struct BuyTickets_locals
	{
		/** Purchase input stored by this structure. */
		ExecuteTicketPurchase_input purchaseInput;
		/** Purchase output stored by this structure. */
		ExecuteTicketPurchase_output purchaseOutput;
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
		/** Wallet debited by the creator-operation fee. */
		CreatorWallet wallet;
		/** Request passed to the refund helper. */
		RefundInvocationReward_input refundInput;
		/** Response returned by the refund helper. */
		RefundInvocationReward_output refundOutput;
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

	/** QPI scratch state for unique-player paging; contract routines cannot declare stack locals. */
	struct GetPlayers_locals
	{
		/** First ticket link for each occupied exact-identity hash slot. */
		Array<uint32, PLDT_PLAYER_LOOKUP_CAPACITY> firstTicketLinks;
		/** Working copy of the ticket currently being traversed. */
		Ticket ticket;
		/** First ticket used to verify identity after a hash collision. */
		Ticket firstTicket;
		/** Working aggregate copied out of and back into the output array. */
		PlayerSummary summary;
		/** Request passed to the round-ticket-list lookup helper. */
		FindGameTicketList_input findInput;
		/** Response returned by the round-ticket-list lookup helper. */
		FindGameTicketList_output findOutput;
		/** Current one-based ticket link during list traversal. */
		uint64 link;
		/** Exact-identity hash slot used by linear probing. */
		uint64 hashSlot;
		/** One-based first-ticket link read from an occupied hash slot. */
		uint64 firstLink;
		/** Stable unique-player index assigned at first occurrence. */
		uint64 playerIndex;
		/** Cursor over summaries in the current output page. */
		uint64 i;
		/** Requested limit after applying the fixed response capacity. */
		uint16 effectiveLimit;
		/** Whether the current ticket's player was already observed. */
		bit foundPlayer;
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

	/** Input values consumed by the freeze finalization state helper. */
	struct FreezeFinalizationState_input
	{
		/** Game stored by this structure. */
		Game game;
		/** Bounded storage slot stored by this structure. */
		uint16 slot;
		/** Reason stored by this structure. */
		EGameTerminalReason reason;
	};

	/** Result values produced by the freeze finalization state helper. */
	struct FreezeFinalizationState_output
	{
		/** Game stored by this structure. */
		Game game;
	};

	/** Input values consumed by the resolve next round schedule helper. */
	struct ResolveNextRoundSchedule_input
	{
		/** Game stored by this structure. */
		Game game;
		/** Bounded storage slot stored by this structure. */
		uint16 slot;
	};

	/** Result values produced by the resolve next round schedule helper. */
	struct ResolveNextRoundSchedule_output
	{
		/** Game stored by this structure. */
		Game game;
		/** Next start at stored by this structure. */
		DateAndTime nextStartAt;
		/** Next draw at stored by this structure. */
		DateAndTime nextDrawAt;
	};

	/** Input values consumed by the drain finalization payouts helper. */
	struct DrainFinalizationPayouts_input
	{
		/** Game stored by this structure. */
		Game game;
		/** Bounded storage slot stored by this structure. */
		uint16 slot;
	};

	/** Result values produced by the drain finalization payouts helper. */
	struct DrainFinalizationPayouts_output
	{
		/** Game stored by this structure. */
		Game game;
		/** Operation result code stored by this structure. */
		EReturnCode returnCode;
	};

	/** QPI scratch state used by the drain finalization payouts helper. */
	struct DrainFinalizationPayouts_locals
	{
		/** Wallet stored by this structure. */
		CreatorWallet wallet;
		/** Wallet asset stored by this structure. */
		WalletAssetBalance walletAsset;
		/** Service credit returned stored by this structure. */
		uint64 serviceCreditReturned;
		/** Capacity currently available in the destination wallet bucket. */
		uint64 availableCredit;
		/** Bounded amount committed during the current retry. */
		uint64 amountToCredit;
		/** Remaining amount in the current creator liability. */
		uint64 pendingAmount;
		/** Bounded-loop index stored by this structure. */
		uint64 i;
		/** Transfer result stored by this structure. */
		sint64 transferResult;
		/** Selects creator fees first, then returned pool and balance. */
		uint8 payoutIndex;
		/** Wallet asset found stored by this structure. */
		bit walletAssetFound;
	};

	/** Input values consumed by the validate round fee capacity helper. */
	struct ValidateRoundFeeCapacity_input
	{
		/** Game stored by this structure. */
		Game game;
	};

	/** Result values produced by the validate round fee capacity helper. */
	struct ValidateRoundFeeCapacity_output
	{
		/** Developer1 fee stored by this structure. */
		uint64 developer1Fee;
		/** Developer2 fee stored by this structure. */
		uint64 developer2Fee;
		/** Dividend fee stored by this structure. */
		uint64 dividendFee;
		/** Operation result code stored by this structure. */
		EReturnCode returnCode;
	};

	/** Input values consumed by the build round result helper. */
	struct BuildRoundResult_input
	{
		/** Progress stored by this structure. */
		SettlementProgress progress;
		/** Game stored by this structure. */
		Game game;
	};

	/** Result values produced by the build round result helper. */
	struct BuildRoundResult_output
	{
		/** Result stored by this structure. */
		GameResult result;
	};

	/** Input values consumed by the commit finalized round helper. */
	struct CommitFinalizedRound_input
	{
		/** Progress stored by this structure. */
		SettlementProgress progress;
		/** Result stored by this structure. */
		GameResult result;
		/** Game stored by this structure. */
		Game game;
		/** Next start at stored by this structure. */
		DateAndTime nextStartAt;
		/** Next draw at stored by this structure. */
		DateAndTime nextDrawAt;
		/** Fees stored by this structure. */
		ValidateRoundFeeCapacity_output fees;
		/** Bounded storage slot stored by this structure. */
		uint16 slot;
	};

	/** Result values produced by the commit finalized round helper. */
	struct CommitFinalizedRound_output
	{
		/** Operation result code stored by this structure. */
		EReturnCode returnCode;
	};

	/** QPI scratch state used by the commit finalized round helper. */
	struct CommitFinalizedRound_locals
	{
		/** Clear input stored by this structure. */
		ClearGameSlot_input clearInput;
		/** Clear output stored by this structure. */
		ClearGameSlot_output clearOutput;
		/** Progress stored by this structure. */
		SettlementProgress progress;
		/** Game stored by this structure. */
		Game game;
		/** Result index stored by this structure. */
		uint64 resultIndex;
	};

	/** QPI scratch state for finalize game; contract routines cannot declare stack locals. */
	struct FinalizeGame_locals
	{
		/** Freeze input stored by this structure. */
		FreezeFinalizationState_input freezeInput;
		/** Freeze output stored by this structure. */
		FreezeFinalizationState_output freezeOutput;
		/** Schedule input stored by this structure. */
		ResolveNextRoundSchedule_input scheduleInput;
		/** Schedule output stored by this structure. */
		ResolveNextRoundSchedule_output scheduleOutput;
		/** Payouts input stored by this structure. */
		DrainFinalizationPayouts_input payoutsInput;
		/** Payouts output stored by this structure. */
		DrainFinalizationPayouts_output payoutsOutput;
		/** Fee input stored by this structure. */
		ValidateRoundFeeCapacity_input feeInput;
		/** Fee output stored by this structure. */
		ValidateRoundFeeCapacity_output feeOutput;
		/** Result input stored by this structure. */
		BuildRoundResult_input resultInput;
		/** Result output stored by this structure. */
		BuildRoundResult_output resultOutput;
		/** Commit input stored by this structure. */
		CommitFinalizedRound_input commitInput;
		/** Commit output stored by this structure. */
		CommitFinalizedRound_output commitOutput;
		/** Progress stored by this structure. */
		SettlementProgress progress;
		/** Previous result stored by this structure. */
		GameResult previousResult;
		/** Game stored by this structure. */
		Game game;
		/** Result index stored by this structure. */
		uint64 resultIndex;
	};

	/** QPI scratch state for clear game slot; contract routines cannot declare stack locals. */
	struct ClearGameSlot_locals
	{
		/** Working copy of the game currency asset's accrual bucket. */
		AssetPlatformAccounting assetAccounting;
		/** Working copy or returned snapshot of a game record. */
		Game game;
		/** Wallet stored by this structure. */
		CreatorWallet wallet;
		/** Wallet asset stored by this structure. */
		WalletAssetBalance walletAsset;
		/** Bounded-loop index stored by this structure. */
		uint64 i;
		/** Wallet asset slot stored by this structure. */
		uint8 walletAssetSlot;
		/** Wallet asset found stored by this structure. */
		bit walletAssetFound;
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

	/** Input values consumed by the classify settlement tickets helper. */
	struct ClassifySettlementTickets_input
	{
		/** Progress stored by this structure. */
		SettlementProgress progress;
		/** Game stored by this structure. */
		Game game;
		/** Remaining bounded-work budget stored by this structure. */
		uint64 budget;
	};

	/** Result values produced by the classify settlement tickets helper. */
	struct ClassifySettlementTickets_output
	{
		/** Progress stored by this structure. */
		SettlementProgress progress;
		/** Game stored by this structure. */
		Game game;
		/** Remaining bounded-work budget stored by this structure. */
		uint64 budget;
	};

	/** QPI scratch state used by the classify settlement tickets helper. */
	struct ClassifySettlementTickets_locals
	{
		/** Match input stored by this structure. */
		CountMatches_input matchInput;
		/** Match output stored by this structure. */
		CountMatches_output matchOutput;
		/** Ticket stored by this structure. */
		Ticket ticket;
		/** One-based storage link stored by this structure. */
		uint64 link;
	};

	/** Input values consumed by the allocate settlement payouts helper. */
	struct AllocateSettlementPayouts_input
	{
		/** Progress stored by this structure. */
		SettlementProgress progress;
		/** Game stored by this structure. */
		Game game;
	};

	/** Result values produced by the allocate settlement payouts helper. */
	struct AllocateSettlementPayouts_output
	{
		/** Progress stored by this structure. */
		SettlementProgress progress;
		/** Game stored by this structure. */
		Game game;
		/** No winners stored by this structure. */
		bit noWinners;
	};

	/** QPI scratch state used by the allocate settlement payouts helper. */
	struct AllocateSettlementPayouts_locals
	{
		/** Count stored by this structure. */
		uint64 count;
		/** Bonus count stored by this structure. */
		uint64 bonusCount;
		/** Total weight stored by this structure. */
		uint64 totalWeight;
		/** Tier pool stored by this structure. */
		uint64 tierPool;
		/** Floor sum stored by this structure. */
		uint64 floorSum;
		/** Remainder stored by this structure. */
		uint64 remainder;
		/** Fraction stored by this structure. */
		uint64 fraction;
		/** Best fraction stored by this structure. */
		uint64 bestFraction;
		/** Bounded-loop index stored by this structure. */
		uint64 i;
		/** Best tier stored by this structure. */
		uint16 bestTier;
		/** Found tier stored by this structure. */
		bit foundTier;
	};

	/** Input values consumed by the pay settlement tickets helper. */
	struct PaySettlementTickets_input
	{
		/** Progress stored by this structure. */
		SettlementProgress progress;
		/** Game stored by this structure. */
		Game game;
		/** Remaining bounded-work budget stored by this structure. */
		uint64 budget;
	};

	/** Result values produced by the pay settlement tickets helper. */
	struct PaySettlementTickets_output
	{
		/** Progress stored by this structure. */
		SettlementProgress progress;
		/** Game stored by this structure. */
		Game game;
		/** Remaining bounded-work budget stored by this structure. */
		uint64 budget;
		/** Operation result code stored by this structure. */
		EReturnCode returnCode;
	};

	/** QPI scratch state used by the pay settlement tickets helper. */
	struct PaySettlementTickets_locals
	{
		/** Request passed to the wallet asset credit helper. */
		CreditWalletAsset_input creditInput;
		/** Response returned by the wallet asset credit helper. */
		CreditWalletAsset_output creditOutput;
		/** Transfer input stored by this structure. */
		TransferGameCurrency_input transferInput;
		/** Transfer output stored by this structure. */
		TransferGameCurrency_output transferOutput;
		/** Ticket stored by this structure. */
		Ticket ticket;
		/** One-based storage link stored by this structure. */
		uint64 link;
		/** Payout stored by this structure. */
		uint64 payout;
	};

	/** QPI scratch state for advance settlement; contract routines cannot declare stack locals. */
	struct AdvanceSettlement_locals
	{
		/** Classify input stored by this structure. */
		ClassifySettlementTickets_input classifyInput;
		/** Classify output stored by this structure. */
		ClassifySettlementTickets_output classifyOutput;
		/** Allocate input stored by this structure. */
		AllocateSettlementPayouts_input allocateInput;
		/** Allocate output stored by this structure. */
		AllocateSettlementPayouts_output allocateOutput;
		/** Pay input stored by this structure. */
		PaySettlementTickets_input payInput;
		/** Pay output stored by this structure. */
		PaySettlementTickets_output payOutput;
		/** Finalize input stored by this structure. */
		FinalizeGame_input finalizeInput;
		/** Finalize output stored by this structure. */
		FinalizeGame_output finalizeOutput;
		/** Progress stored by this structure. */
		SettlementProgress progress;
		/** Game stored by this structure. */
		Game game;
		/** Remaining bounded-work budget stored by this structure. */
		uint64 budget;
		/** Bounded storage slot stored by this structure. */
		uint16 slot;
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
		/** Wallet stored by this structure. */
		CreatorWallet wallet;
		/** Wallet asset stored by this structure. */
		WalletAssetBalance walletAsset;
		/** Wallet owner stored by this structure. */
		id walletOwner;
		/** Maximum state-changing work allowed in this invocation. */
		uint64 actionBudget;
		/** Wallet index stored by this structure. */
		uint64 walletIndex;
		/** Developer1 fee stored by this structure. */
		uint64 developer1Fee;
		/** Developer2 fee stored by this structure. */
		uint64 developer2Fee;
		/** Dividend fee stored by this structure. */
		uint64 dividendFee;
		/** Transfer result stored by this structure. */
		sint64 transferResult;
		/** Number of game slots examined during this automation pass. */
		uint16 inspected;
		/** Wallets inspected stored by this structure. */
		uint16 walletsInspected;
		/** Zero-based game, ticket, result, or accounting slot under operation. */
		uint16 slot;
		/** Wallet actions stored by this structure. */
		uint8 walletActions;
	};

	/** QPI scratch state for platform configuration; contract routines cannot declare stack locals. */
	struct SetPlatformConfig_locals
	{
		/** Request passed to the refund helper. */
		RefundInvocationReward_input refundInput;
		/** Response returned by the refund helper. */
		RefundInvocationReward_output refundOutput;
	};

	/** QPI scratch state for withdraw platform revenue; contract routines cannot declare stack locals. */
	struct WithdrawPlatformRevenue_locals
	{
		/** QPI transfer result; a negative value indicates failure. */
		sint64 transferResult;
		/** Request passed to the refund helper. */
		RefundInvocationReward_input refundInput;
		/** Response returned by the refund helper. */
		RefundInvocationReward_output refundOutput;
		/** Whether any resumable transfer attempted in this operation failed. */
		bit failed;
	};

	REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
	{
		REGISTER_USER_PROCEDURE(CreateGame, 1);
		REGISTER_USER_PROCEDURE(CreateWallet, 2);
		REGISTER_USER_PROCEDURE(FundGame, 3);
		REGISTER_USER_PROCEDURE(BuyTicket, 4);
		REGISTER_USER_PROCEDURE(UpdateGameEconomics, 5);
		REGISTER_USER_PROCEDURE(DepositWalletQubic, 6);
		REGISTER_USER_PROCEDURE(StopGame, 7);
		REGISTER_USER_PROCEDURE(SetPlatformConfig, 8);
		REGISTER_USER_PROCEDURE(WithdrawPlatformRevenue, 9);
		REGISTER_USER_PROCEDURE(WithdrawGameBalance, 10);
		REGISTER_USER_PROCEDURE(TransferShareManagementRights, 11);
		REGISTER_USER_PROCEDURE(WithdrawWalletQubic, 12);
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
		REGISTER_USER_FUNCTION(GetWallet, 10);
		REGISTER_USER_FUNCTION(GetPlayers, 11);
	}

	INITIALIZE()
	{
		state.mut().platformOwner =
		    ID(_R, _O, _J, _V, _A, _E, _M, _F, _B, _X, _X, _Y, _N, _G, _A, _U, _A, _U, _I, _I, _X, _L, _B, _U, _P, _D, _H, _C, _D, _P, _E, _S, _Y, _Z,
		       _O, _V, _W, _U, _Y, _E, _C, _B, _Q, _V, _Z, _R, _F, _T, _K, _A, _G, _S, _H, _T, _N, _A);
		state.mut().platformFeePercent = PLDT_PLATFORM_FEE_PERCENT;
		state.mut().maxCreatorFeePercent = PLDT_DEFAULT_MAX_CREATOR_FEE_PERCENT;
		state.mut().roundFee = PLDT_DEFAULT_ROUND_FEE;
		state.mut().walletCreationFee = PLDT_DEFAULT_WALLET_CREATION_FEE;
	}

	PRE_ACQUIRE_SHARES_WITH_LOCALS()
	{
		output.requestedFee = 0;
		output.allowTransfer = false;
		if (qpi.originator() != input.owner || qpi.originator() != input.possessor || input.numberOfShares <= 0)
		{
			return;
		}
		if (!state.get().wallets.get(input.owner, locals.wallet))
		{
			output.allowTransfer = true;
			return;
		}
		if (locals.wallet.status != EWalletStatus::OPEN)
		{
			return;
		}
		locals.found = findWalletAsset(locals.wallet, input.asset, locals.i, locals.assetBalance);
		if (locals.found)
		{
			if (locals.assetBalance.balance <= PLDT_MAX_TRANSFER_AMOUNT - static_cast<uint64>(input.numberOfShares))
			{
				output.allowTransfer = true;
			}
			return;
		}
		output.allowTransfer = locals.wallet.assetCount < PLDT_MAX_WALLET_ASSETS;
	}

	POST_ACQUIRE_SHARES_WITH_LOCALS()
	{
		if (input.numberOfShares <= 0)
		{
			return;
		}
		locals.creditInput.owner = input.owner;
		locals.creditInput.asset = input.asset;
		locals.creditInput.amount = static_cast<uint64>(input.numberOfShares);
		CALL(CreditWalletAsset, locals.creditInput, locals.creditOutput);
	}

	BEGIN_TICK_WITH_LOCALS()
	{
		if (mod(qpi.tick(), PLDT_TICK_UPDATE_PERIOD) != 0)
		{
			return;
		}

		locals.actionBudget = PLDT_SETTLEMENT_ACTION_BUDGET;
		for (locals.inspected = 0; locals.inspected < PLDT_AUTOMATION_GAMES_PER_TICK; ++locals.inspected)
		{
			locals.slot = automationGameSlot(locals.inspected, state.get().automationCursor);
			if (state.get().games.get(locals.slot).status != EGameStatus::EMPTY_SLOT)
			{
				locals.processInput.slot = locals.slot;
				locals.processInput.actionBudget = locals.actionBudget;
				CALL(ProcessGame, locals.processInput, locals.processOutput);
				locals.actionBudget -= locals.processOutput.actionsUsed;
			}
		}

		state.mut().automationCursor = automationGameSlot(locals.inspected, state.get().automationCursor);
		locals.reclaimInput.actionBudget = locals.actionBudget;
		CALL(ReclaimCompletedTickets, locals.reclaimInput, locals.reclaimOutput);

		locals.walletsInspected = 0;
		locals.walletActions = 0;
		while (locals.walletsInspected < PLDT_AUTOMATION_WALLETS_PER_TICK)
		{
			locals.walletIndex =
			    mod(static_cast<uint64>(state.get().walletAutomationCursor + locals.walletsInspected), static_cast<uint64>(PLDT_WALLET_MAP_CAPACITY));
			++locals.walletsInspected;
			if (state.get().wallets.isEmptySlot(static_cast<sint64>(locals.walletIndex)))
			{
				continue;
			}
			locals.walletOwner = state.get().wallets.key(static_cast<sint64>(locals.walletIndex));
			locals.wallet = state.get().wallets.value(static_cast<sint64>(locals.walletIndex));
			if (locals.wallet.status == EWalletStatus::OPEN && locals.wallet.activeGameCount == 0 &&
			    epochElapsedAtLeast(qpi.epoch(), locals.wallet.lastGameCreationEpoch, 2))
			{
				locals.wallet.status = EWalletStatus::EXPIRING;
				state.mut().wallets.replace(locals.walletOwner, locals.wallet);
			}
			while (locals.wallet.status == EWalletStatus::EXPIRING && locals.wallet.expiryAssetCursor < PLDT_MAX_WALLET_ASSETS &&
			       locals.walletActions < PLDT_WALLET_EXPIRY_ASSET_ACTION_BUDGET)
			{
				locals.walletAsset = locals.wallet.assets.get(locals.wallet.expiryAssetCursor);
				++locals.wallet.expiryAssetCursor;
				if (!locals.walletAsset.isActive)
				{
					continue;
				}
				if (locals.walletAsset.balance > 0)
				{
					locals.transferResult = qpi.releaseShares(locals.walletAsset.asset, locals.walletOwner, locals.walletOwner,
					                                          static_cast<sint64>(locals.walletAsset.balance), QX_CONTRACT_INDEX, QX_CONTRACT_INDEX,
					                                          static_cast<sint64>(locals.wallet.serviceCredit));
					if (locals.transferResult >= 0 && static_cast<uint64>(locals.transferResult) <= locals.wallet.serviceCredit)
					{
						locals.wallet.serviceCredit -= static_cast<uint64>(locals.transferResult);
					}
				}
				setMemory(locals.walletAsset, 0);
				locals.wallet.assets.set(locals.wallet.expiryAssetCursor - 1, locals.walletAsset);
				if (locals.wallet.assetCount > 0)
				{
					--locals.wallet.assetCount;
				}
				++locals.walletActions;
				state.mut().wallets.replace(locals.walletOwner, locals.wallet);
			}
			if (locals.wallet.status != EWalletStatus::EXPIRING || locals.wallet.assetCount != 0)
			{
				continue;
			}
			if (locals.wallet.serviceCreditUnlocked && locals.wallet.serviceCredit > 0)
			{
				locals.transferResult = qpi.transfer(locals.walletOwner, static_cast<sint64>(locals.wallet.serviceCredit));
				if (locals.transferResult < 0)
				{
					continue;
				}
				locals.wallet.serviceCredit = 0;
				state.mut().wallets.replace(locals.walletOwner, locals.wallet);
			}
			calculatePlatformShares(locals.wallet.serviceCredit, locals.developer1Fee, locals.developer2Fee, locals.dividendFee);
			if (!hasPlatformAccrualCapacity(state.get().developer1Accrued, state.get().developer2Accrued, state.get().dividendAccrued,
			                                locals.developer1Fee, locals.developer2Fee, locals.dividendFee))
			{
				continue;
			}
			if (locals.wallet.refundableQubic > 0)
			{
				locals.transferResult = qpi.transfer(locals.walletOwner, static_cast<sint64>(locals.wallet.refundableQubic));
				if (locals.transferResult < 0)
				{
					continue;
				}
				locals.wallet.refundableQubic = 0;
			}
			state.mut().developer1Accrued = sadd(state.get().developer1Accrued, locals.developer1Fee);
			state.mut().developer2Accrued = sadd(state.get().developer2Accrued, locals.developer2Fee);
			state.mut().dividendAccrued = sadd(state.get().dividendAccrued, locals.dividendFee);
			state.mut().wallets.removeByKey(locals.walletOwner);
		}
		state.mut().wallets.cleanupIfNeeded();
		state.mut().walletAutomationCursor = static_cast<uint16>(
		    mod(static_cast<uint64>(state.get().walletAutomationCursor + locals.walletsInspected), static_cast<uint64>(PLDT_WALLET_MAP_CAPACITY)));
	}

	/**
	 * @brief Opens a creator wallet funded by the invocation reward.
	 * @param input Empty input; the invocator becomes the wallet owner.
	 * @param output Service-credit and refundable-Qubic balances plus the result code.
	 * @note The configured creation fee is restricted service credit until the owner's last active game closes.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(CreateWallet)
	{
		if (state.get().wallets.get(qpi.invocator(), locals.wallet))
		{
			if (qpi.invocationReward() > 0)
			{
				locals.transferResult = qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.returnCode = locals.transferResult < 0 ? EReturnCode::TRANSFER_FAILED : EReturnCode::INVALID_STATE;
			return;
		}
		if (qpi.invocationReward() < static_cast<sint64>(state.get().walletCreationFee))
		{
			if (qpi.invocationReward() > 0)
			{
				locals.transferResult = qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.returnCode = locals.transferResult < 0 ? EReturnCode::TRANSFER_FAILED : EReturnCode::INSUFFICIENT_FUNDS;
			return;
		}
		if (state.get().wallets.population() >= PLDT_MAX_WALLETS)
		{
			locals.transferResult = qpi.transfer(qpi.invocator(), qpi.invocationReward());
			output.returnCode = locals.transferResult < 0 ? EReturnCode::TRANSFER_FAILED : EReturnCode::STORAGE_FULL;
			return;
		}
		setMemory(locals.wallet, 0);
		locals.wallet.serviceCredit = state.get().walletCreationFee;
		locals.wallet.refundableQubic = static_cast<uint64>(qpi.invocationReward()) - state.get().walletCreationFee;
		locals.wallet.lastGameCreationEpoch = qpi.epoch();
		locals.wallet.status = EWalletStatus::OPEN;
		locals.mapIndex = state.mut().wallets.set(qpi.invocator(), locals.wallet);
		if (locals.mapIndex < 0)
		{
			locals.transferResult = qpi.transfer(qpi.invocator(), qpi.invocationReward());
			output.returnCode = locals.transferResult < 0 ? EReturnCode::TRANSFER_FAILED : EReturnCode::STORAGE_FULL;
			return;
		}
		output.serviceCredit = locals.wallet.serviceCredit;
		output.refundableQubic = locals.wallet.refundableQubic;
		output.returnCode = EReturnCode::SUCCESS;
	}

	/**
	 * @brief Returns a public snapshot of a creator wallet.
	 * @param input Owner identity to query.
	 * @param output Wallet balances, asset positions, lifecycle fields, and presence flag.
	 */
	PUBLIC_FUNCTION_WITH_LOCALS(GetWallet)
	{
		output.found = state.get().wallets.get(input.owner, locals.wallet);
		if (!output.found)
		{
			return;
		}
		output.assets = locals.wallet.assets;
		output.serviceCredit = locals.wallet.serviceCredit;
		output.refundableQubic = locals.wallet.refundableQubic;
		output.activeGameCount = locals.wallet.activeGameCount;
		output.lastGameCreationEpoch = locals.wallet.lastGameCreationEpoch;
		output.assetCount = locals.wallet.assetCount;
		output.status = locals.wallet.status;
		output.serviceCreditUnlocked = locals.wallet.serviceCreditUnlocked && locals.wallet.activeGameCount == 0;
	}

	/**
	 * @brief Deposits the full invocation reward as refundable creator-wallet Qubic.
	 * @param input Empty input; the invocator's open wallet is credited.
	 * @param output Updated refundable balance and result code.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(DepositWalletQubic)
	{
		if (!state.get().wallets.get(qpi.invocator(), locals.wallet) || locals.wallet.status != EWalletStatus::OPEN)
		{
			if (qpi.invocationReward() > 0)
			{
				locals.transferResult = qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.returnCode = locals.transferResult < 0 ? EReturnCode::TRANSFER_FAILED : EReturnCode::INVALID_STATE;
			return;
		}
		if (qpi.invocationReward() <= 0 || locals.wallet.refundableQubic > PLDT_MAX_TRANSFER_AMOUNT - static_cast<uint64>(qpi.invocationReward()))
		{
			if (qpi.invocationReward() > 0)
			{
				locals.transferResult = qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.returnCode = locals.transferResult < 0 ? EReturnCode::TRANSFER_FAILED : EReturnCode::INVALID_VALUE;
			return;
		}
		locals.wallet.refundableQubic = sadd(locals.wallet.refundableQubic, static_cast<uint64>(qpi.invocationReward()));
		state.mut().wallets.replace(qpi.invocator(), locals.wallet);
		output.refundableQubic = locals.wallet.refundableQubic;
		output.returnCode = EReturnCode::SUCCESS;
	}

	/**
	 * @brief Returns withdrawable Qubic from the invocator's creator wallet.
	 * @param input Amount to withdraw; unlocked service credit is consumed before refundable Qubic.
	 * @param output Successfully paid amount and result code.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(WithdrawWalletQubic)
	{
		if (qpi.invocationReward() != 0)
		{
			locals.transferResult = qpi.transfer(qpi.invocator(), qpi.invocationReward());
			output.returnCode = locals.transferResult < 0 ? EReturnCode::TRANSFER_FAILED : EReturnCode::INVALID_VALUE;
			return;
		}
		if (!state.get().wallets.get(qpi.invocator(), locals.wallet) || locals.wallet.status != EWalletStatus::OPEN)
		{
			output.returnCode = EReturnCode::INVALID_STATE;
			return;
		}
		if (input.amount == 0)
		{
			output.returnCode = EReturnCode::INVALID_VALUE;
			return;
		}
		if ((!locals.wallet.serviceCreditUnlocked || locals.wallet.activeGameCount != 0) && input.amount > locals.wallet.refundableQubic)
		{
			output.returnCode = EReturnCode::INSUFFICIENT_FUNDS;
			return;
		}
		if (locals.wallet.serviceCreditUnlocked && locals.wallet.activeGameCount == 0 &&
		    input.amount > locals.wallet.serviceCredit && input.amount - locals.wallet.serviceCredit > locals.wallet.refundableQubic)
		{
			output.returnCode = EReturnCode::INSUFFICIENT_FUNDS;
			return;
		}
		locals.transferResult = qpi.transfer(qpi.invocator(), static_cast<sint64>(input.amount));
		if (locals.transferResult < 0)
		{
			output.returnCode = EReturnCode::TRANSFER_FAILED;
			return;
		}
		if (locals.wallet.serviceCreditUnlocked && locals.wallet.activeGameCount == 0)
		{
			if (input.amount <= locals.wallet.serviceCredit)
			{
				locals.wallet.serviceCredit -= input.amount;
			}
			else
			{
				locals.wallet.refundableQubic -= input.amount - locals.wallet.serviceCredit;
				locals.wallet.serviceCredit = 0;
			}
		}
		else
		{
			locals.wallet.refundableQubic -= input.amount;
		}
		state.mut().wallets.replace(qpi.invocator(), locals.wallet);
		output.amountPaid = input.amount;
		output.returnCode = EReturnCode::SUCCESS;
	}

	/**
	 * @brief Validates a proposed game and previews its initial funding and accounting.
	 * @param input Complete game configuration, lifecycle mode, and initial ledgers.
	 * @param output Required funding, per-ticket economics, and validation result.
	 */
	PUBLIC_FUNCTION_WITH_LOCALS(PreviewGame)
	{
		locals.configurationInput.configuration = input;
		CALL(ValidateGameConfiguration, locals.configurationInput, locals.configurationOutput);
		if (locals.configurationOutput.returnCode != EReturnCode::SUCCESS)
		{
			output.returnCode = locals.configurationOutput.returnCode;
			return;
		}
		locals.tierInput.tierWeightsBps = input.tierWeightsBps;
		locals.tierInput.codeLength = input.codeLength;
		CALL(ValidateTierConfiguration, locals.tierInput, locals.tierOutput);
		if (locals.tierOutput.returnCode != EReturnCode::SUCCESS)
		{
			output.returnCode = locals.tierOutput.returnCode;
			return;
		}
		locals.previewInput.configuration = input;
		CALL(CalculateGamePreview, locals.previewInput, locals.previewOutput);
		output = locals.previewOutput.preview;
	}

	/**
	 * @brief Atomically creates, funds, and charges a one-shot or permanent game.
	 * @param input Game rules, lifecycle mode, UTC window, economics, currency, and initial ledgers.
	 * @param output Generation-aware game id, slot, and result code.
	 * @note An authorized call burns the operation fee before business validation; the first round fee is also non-refundable.
	 * @note Creator fees and returned pools credit the internal wallet at finalization; future rounds use prefunded game balances.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(CreateGame)
	{
		output.gameId = 0;
		output.slot = 0;
		if (qpi.invocationReward() != 0)
		{
			locals.refundInput.returnCode = EReturnCode::TICKET_INVALID_PRICE;
			CALL(RefundInvocationReward, locals.refundInput, locals.refundOutput);
			output.returnCode = locals.refundOutput.returnCode;
			return;
		}
		if (!state.get().wallets.get(qpi.invocator(), locals.wallet) || locals.wallet.status != EWalletStatus::OPEN)
		{
			output.returnCode = EReturnCode::INVALID_STATE;
			return;
		}
		if (!debitWalletOperationFee(locals.wallet))
		{
			output.returnCode = EReturnCode::INSUFFICIENT_FUNDS;
			return;
		}
		if (qpi.burn(static_cast<sint64>(PLDT_OPERATION_FEE)) < 0)
		{
			output.returnCode = EReturnCode::TRANSFER_FAILED;
			return;
		}
		state.mut().wallets.replace(qpi.invocator(), locals.wallet);
		locals.previewInput = input;
		CALL(PreviewGame, locals.previewInput, locals.previewOutput);
		if (locals.previewOutput.returnCode != EReturnCode::SUCCESS)
		{
			locals.refundInput.returnCode = locals.previewOutput.returnCode;
			CALL(RefundInvocationReward, locals.refundInput, locals.refundOutput);
			output.returnCode = locals.refundOutput.returnCode;
			return;
		}
		locals.prepareInput.configuration = input;
		locals.prepareInput.preview = locals.previewOutput;
		CALL(PrepareGameCreation, locals.prepareInput, locals.prepareOutput);
		if (locals.prepareOutput.returnCode != EReturnCode::SUCCESS)
		{
			locals.refundInput.returnCode = locals.prepareOutput.returnCode;
			CALL(RefundInvocationReward, locals.refundInput, locals.refundOutput);
			output.returnCode = locals.refundOutput.returnCode;
			return;
		}
		locals.fundingInput.configuration = input;
		CALL(CollectInitialAssetFunding, locals.fundingInput, locals.fundingOutput);
		if (locals.fundingOutput.returnCode != EReturnCode::SUCCESS)
		{
			locals.refundInput.returnCode = locals.fundingOutput.returnCode;
			CALL(RefundInvocationReward, locals.refundInput, locals.refundOutput);
			output.returnCode = locals.refundOutput.returnCode;
			return;
		}
		locals.commitInput.configuration = input;
		locals.commitInput.prepared = locals.prepareOutput;
		CALL(CommitCreatedGame, locals.commitInput, locals.commitOutput);
		output.gameId = locals.commitOutput.gameId;
		output.slot = locals.commitOutput.slot;
		output.returnCode = locals.commitOutput.returnCode;
	}

	/**
	 * @brief Adds Qubic run credit and/or game-currency creator balance.
	 * @param input Game id and ledger top-ups.
	 * @param output Result code; failed business validation preserves game ledgers.
	 * @note An authorized call burns the operation fee before business validation.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(FundGame)
	{
		// Resolve ownership and wallet funding before changing either currency ledger.
		locals.refundInput.returnCode = resolveOwnedGame(state, input.gameId, qpi.invocator(), locals.slot, locals.game);
		if (locals.refundInput.returnCode != EReturnCode::SUCCESS)
		{
			CALL(RefundInvocationReward, locals.refundInput, locals.refundOutput);
			output.returnCode = locals.refundOutput.returnCode;
			return;
		}
		if (!state.get().wallets.get(qpi.invocator(), locals.wallet) || locals.wallet.status != EWalletStatus::OPEN)
		{
			locals.refundInput.returnCode = EReturnCode::INVALID_STATE;
			CALL(RefundInvocationReward, locals.refundInput, locals.refundOutput);
			output.returnCode = locals.refundOutput.returnCode;
			return;
		}
		if (qpi.invocationReward() != 0)
		{
			locals.refundInput.returnCode = EReturnCode::TICKET_INVALID_PRICE;
			CALL(RefundInvocationReward, locals.refundInput, locals.refundOutput);
			output.returnCode = locals.refundOutput.returnCode;
			return;
		}
		if (!debitWalletOperationFee(locals.wallet))
		{
			output.returnCode = EReturnCode::INSUFFICIENT_FUNDS;
			return;
		}
		if (qpi.burn(static_cast<sint64>(PLDT_OPERATION_FEE)) < 0)
		{
			output.returnCode = EReturnCode::TRANSFER_FAILED;
			return;
		}
		state.mut().wallets.replace(qpi.invocator(), locals.wallet);
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
		locals.serviceCreditDebit = locals.wallet.serviceCredit < input.runCreditTopUp ? locals.wallet.serviceCredit : input.runCreditTopUp;
		if (locals.expectedReward - locals.serviceCreditDebit > locals.wallet.refundableQubic)
		{
			output.returnCode = EReturnCode::INSUFFICIENT_FUNDS;
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
		// Asset custody is the only fallible transfer and therefore precedes the ledger commit.
		if (locals.game.currencyMode == ECurrencyMode::ASSET && input.creatorBalanceTopUp > 0)
		{
			locals.walletAssetFound = findWalletAsset(locals.wallet, locals.game.currencyAsset, locals.i, locals.walletAsset);
			if (locals.walletAssetFound)
			{
				locals.walletAssetSlot = static_cast<uint8>(locals.i);
			}
			if (!locals.walletAssetFound || locals.walletAsset.balance < input.creatorBalanceTopUp)
			{
				output.returnCode = EReturnCode::INSUFFICIENT_FUNDS;
				return;
			}
			locals.possessedShares = qpi.numberOfPossessedShares(locals.game.currencyAsset.assetName, locals.game.currencyAsset.issuer,
			                                                     qpi.invocator(), qpi.invocator(), SELF_INDEX, SELF_INDEX);
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
			locals.walletAsset.balance -= input.creatorBalanceTopUp;
			locals.wallet.assets.set(locals.walletAssetSlot, locals.walletAsset);
		}
		// Commit the wallet debit and matching game credits as one state transition.
		locals.wallet.serviceCredit -= locals.serviceCreditDebit;
		locals.wallet.refundableQubic -= locals.expectedReward - locals.serviceCreditDebit;
		locals.game.runCredit = sadd(locals.game.runCredit, input.runCreditTopUp);
		locals.game.creatorBalance = sadd(locals.game.creatorBalance, input.creatorBalanceTopUp);
		locals.game.runServiceCredit = sadd(locals.game.runServiceCredit, locals.serviceCreditDebit);
		state.mut().wallets.replace(qpi.invocator(), locals.wallet);
		state.mut().games.set(locals.slot, locals.game);
		output.returnCode = EReturnCode::SUCCESS;
	}

	/**
	 * @brief Withdraws unreserved game ledgers to their owner.
	 * @param input Requested amounts; the current prize pool is not addressable.
	 * @param output Successfully transferred amounts and result code.
	 * @note An authorized call burns the operation fee before business validation; asset wallet mirroring is best effort.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(WithdrawGameBalance)
	{
		if (qpi.invocationReward() != 0)
		{
			locals.refundInput.returnCode = EReturnCode::INVALID_VALUE;
			CALL(RefundInvocationReward, locals.refundInput, locals.refundOutput);
			output.returnCode = locals.refundOutput.returnCode;
			return;
		}
		output.returnCode = resolveOwnedGame(state, input.gameId, qpi.invocator(), locals.slot, locals.game);
		if (output.returnCode != EReturnCode::SUCCESS)
		{
			return;
		}
		if (!state.get().wallets.get(qpi.invocator(), locals.wallet) || locals.wallet.status != EWalletStatus::OPEN)
		{
			output.returnCode = EReturnCode::INVALID_STATE;
			return;
		}
		if (!debitWalletOperationFee(locals.wallet))
		{
			output.returnCode = EReturnCode::INSUFFICIENT_FUNDS;
			return;
		}
		if (qpi.burn(static_cast<sint64>(PLDT_OPERATION_FEE)) < 0)
		{
			output.returnCode = EReturnCode::TRANSFER_FAILED;
			return;
		}
		state.mut().wallets.replace(qpi.invocator(), locals.wallet);
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
		// Prove the destination wallet can represent every returned QU before moving assets.
		locals.failed = false;
		locals.qubicAmount = input.runCreditAmount;
		if (locals.game.currencyMode == ECurrencyMode::QUBIC)
		{
			locals.qubicAmount = sadd(locals.qubicAmount, input.creatorBalanceAmount);
		}
		if (!prepareWalletQubicCredit(locals.wallet, locals.qubicAmount,
		                              locals.game.runServiceCredit < input.runCreditAmount ? locals.game.runServiceCredit : input.runCreditAmount,
		                              locals.serviceCreditReturned))
		{
			output.returnCode = EReturnCode::STORAGE_FULL;
			return;
		}
		if (locals.game.currencyMode == ECurrencyMode::QUBIC)
		{
			locals.game.creatorBalance -= input.creatorBalanceAmount;
			output.creatorBalancePaid = input.creatorBalanceAmount;
		}
		else
		{
			// Ownership transfer is authoritative; wallet mirroring must not lock funds at fixed-capacity boundaries.
			if (input.creatorBalanceAmount > 0)
			{
				locals.transferResult =
				    qpi.transferShareOwnershipAndPossession(locals.game.currencyAsset.assetName, locals.game.currencyAsset.issuer, SELF, SELF,
				                                            static_cast<sint64>(input.creatorBalanceAmount), locals.game.owner);
				if (locals.transferResult >= 0)
				{
					locals.game.creatorBalance -= input.creatorBalanceAmount;
					output.creatorBalancePaid = input.creatorBalanceAmount;
					locals.walletAssetFound = findWalletAsset(locals.wallet, locals.game.currencyAsset, locals.i, locals.walletAsset);
					if (locals.walletAssetFound && locals.walletAsset.balance <= PLDT_MAX_TRANSFER_AMOUNT - input.creatorBalanceAmount)
					{
						locals.walletAsset.balance = sadd(locals.walletAsset.balance, input.creatorBalanceAmount);
						locals.wallet.assets.set(static_cast<uint8>(locals.i), locals.walletAsset);
					}
				}
				else
				{
					locals.failed = true;
				}
			}
		}
		// Return QU to its original service/refundable buckets and persist both ledgers together.
		locals.wallet.serviceCredit = sadd(locals.wallet.serviceCredit, locals.serviceCreditReturned);
		locals.wallet.refundableQubic = sadd(locals.wallet.refundableQubic, locals.qubicAmount - locals.serviceCreditReturned);
		locals.game.runServiceCredit -= locals.serviceCreditReturned;
		locals.game.runCredit -= input.runCreditAmount;
		output.runCreditPaid = input.runCreditAmount;
		state.mut().wallets.replace(qpi.invocator(), locals.wallet);
		state.mut().games.set(locals.slot, locals.game);
		output.returnCode = locals.failed ? EReturnCode::TRANSFER_FAILED : EReturnCode::SUCCESS;
	}

	/**
	 * @brief Queues validated economics for the next round of a permanent game.
	 * @param input Complete replace-on-write next-round economics.
	 * @param output Result code; the current round is never modified.
	 * @note An authorized call burns the operation fee before business validation.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(UpdateGameEconomics)
	{
		if (qpi.invocationReward() != 0)
		{
			locals.refundInput.returnCode = EReturnCode::INVALID_VALUE;
			CALL(RefundInvocationReward, locals.refundInput, locals.refundOutput);
			output.returnCode = locals.refundOutput.returnCode;
			return;
		}
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
		if (!state.get().wallets.get(qpi.invocator(), locals.wallet) || locals.wallet.status != EWalletStatus::OPEN)
		{
			output.returnCode = EReturnCode::INVALID_STATE;
			return;
		}
		if (!debitWalletOperationFee(locals.wallet))
		{
			output.returnCode = EReturnCode::INSUFFICIENT_FUNDS;
			return;
		}
		if (qpi.burn(static_cast<sint64>(PLDT_OPERATION_FEE)) < 0)
		{
			output.returnCode = EReturnCode::TRANSFER_FAILED;
			return;
		}
		state.mut().wallets.replace(qpi.invocator(), locals.wallet);
		if (locals.game.mode != EGameMode::PERMANENT)
		{
			output.returnCode = EReturnCode::INVALID_STATE;
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
		locals.economicsInput.ticketPrice = input.ticketPrice;
		locals.economicsInput.creatorFeePercent = input.creatorFeePercent;
		locals.economicsInput.platformFeePercent = state.get().platformFeePercent;
		calculateTicketEconomics(locals.economicsInput, locals.economicsOutput);
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
		locals.pending.isSet = true;
		locals.game.pendingEconomics = locals.pending;
		state.mut().games.set(locals.slot, locals.game);
		output.returnCode = EReturnCode::SUCCESS;
	}

	/**
	 * @brief Cancels either mode before first start or requests terminal closure when applicable.
	 * @param input Game id.
	 * @param output Result code; an active one-shot round is unchanged and an active permanent round is not shortened.
	 * @note An authorized call burns the operation fee even when the active one-shot operation is a no-op.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(StopGame)
	{
		if (qpi.invocationReward() != 0)
		{
			locals.refundInput.returnCode = EReturnCode::INVALID_VALUE;
			CALL(RefundInvocationReward, locals.refundInput, locals.refundOutput);
			output.returnCode = locals.refundOutput.returnCode;
			return;
		}
		output.returnCode = resolveOwnedGame(state, input.gameId, qpi.invocator(), locals.slot, locals.game);
		if (output.returnCode != EReturnCode::SUCCESS)
		{
			return;
		}
		if (!state.get().wallets.get(qpi.invocator(), locals.wallet) || locals.wallet.status != EWalletStatus::OPEN)
		{
			output.returnCode = EReturnCode::INVALID_STATE;
			return;
		}
		if (!debitWalletOperationFee(locals.wallet))
		{
			output.returnCode = EReturnCode::INSUFFICIENT_FUNDS;
			return;
		}
		if (qpi.burn(static_cast<sint64>(PLDT_OPERATION_FEE)) < 0)
		{
			output.returnCode = EReturnCode::TRANSFER_FAILED;
			return;
		}
		state.mut().wallets.replace(qpi.invocator(), locals.wallet);
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
			locals.game.pendingCreatorBalancePayout = sadd(locals.game.pendingCreatorBalancePayout, locals.game.creatorBalance);
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
		output.ticketIndex = 0;
		output.prizeContribution = 0;
		locals.purchaseInput.purchase.gameId = input.gameId;
		locals.purchaseInput.purchase.ticketCount = 1;
		locals.purchaseInput.purchase.tickets.set(0, input.digits);
		locals.purchaseInput.validateDigitsBeforePlayerLimit = true;
		CALL(ExecuteTicketPurchase, locals.purchaseInput, locals.purchaseOutput);
		if (locals.purchaseOutput.acceptedCount == 1)
		{
			output.ticketId = locals.purchaseOutput.ticketIds.get(0);
			output.ticketIndex = locals.purchaseOutput.ticketIndexes.get(0);
			output.prizeContribution = locals.purchaseOutput.prizeContribution;
		}
		output.returnCode = locals.purchaseOutput.returnCode;
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
		evaluateGameLifecycle(locals.lifecycleInput, locals.lifecycleOutput);
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
	 * @brief Pages through unique players in one round and aggregates their tickets.
	 * @param input Round key, unique-player offset, and requested limit; limits above 64 are clamped.
	 * @param output Stable first-ticket-order summaries, full unique count, returned count, and result code.
	 * @note This read-only query does not depend on the invocator and does not modify contract state.
	 */
	PUBLIC_FUNCTION_WITH_LOCALS(GetPlayers)
	{
		setMemory(output.players, 0);
		setMemory(output.padding0, 0);
		setMemory(output.padding1, 0);
		output.totalCount = 0;
		output.returnedCount = 0;
		setMemory(locals.firstTicketLinks, 0);
		locals.findInput.gameId = input.roundKey.gameId;
		locals.findInput.roundNumber = input.roundKey.roundNumber;
		CALL(FindGameTicketList, locals.findInput, locals.findOutput);
		if (locals.findOutput.returnCode != EReturnCode::SUCCESS)
		{
			output.returnCode = locals.findOutput.returnCode;
			return;
		}

		locals.effectiveLimit = input.limit < PLDT_PLAYERS_PAGE_CAPACITY ? input.limit : PLDT_PLAYERS_PAGE_CAPACITY;
		locals.link = locals.findOutput.firstTicketLink;
		while (locals.link != 0)
		{
			locals.ticket = state.get().tickets.get(locals.link - 1);
			locals.hashSlot = (locals.ticket.player.u64._0 ^ locals.ticket.player.u64._1 ^ locals.ticket.player.u64._2 ^
			                   locals.ticket.player.u64._3) &
			                  (PLDT_PLAYER_LOOKUP_CAPACITY - 1);
			locals.foundPlayer = false;
			locals.firstLink = locals.firstTicketLinks.get(locals.hashSlot);
			while (locals.firstLink != 0)
			{
				locals.firstTicket = state.get().tickets.get(locals.firstLink - 1);
				if (locals.firstTicket.player == locals.ticket.player)
				{
					locals.foundPlayer = true;
					break;
				}
				locals.hashSlot = (locals.hashSlot + 1) & (PLDT_PLAYER_LOOKUP_CAPACITY - 1);
				locals.firstLink = locals.firstTicketLinks.get(locals.hashSlot);
			}
			if (!locals.foundPlayer)
			{
				locals.firstTicketLinks.set(locals.hashSlot, static_cast<uint32>(locals.link));
				locals.playerIndex = output.totalCount++;
				if (locals.playerIndex >= input.offset && output.returnedCount < locals.effectiveLimit)
				{
					setMemory(locals.summary, 0);
					locals.summary.player = locals.ticket.player;
					output.players.set(output.returnedCount++, locals.summary);
				}
			}
			locals.link = locals.ticket.nextLink;
		}

		locals.link = locals.findOutput.firstTicketLink;
		while (locals.link != 0 && output.returnedCount != 0)
		{
			locals.ticket = state.get().tickets.get(locals.link - 1);
			for (locals.i = 0; locals.i < output.returnedCount; ++locals.i)
			{
				locals.summary = output.players.get(locals.i);
				if (locals.summary.player != locals.ticket.player)
				{
					continue;
				}
				locals.summary.totalPayout = sadd(locals.summary.totalPayout, locals.ticket.payout);
				++locals.summary.ticketCount;
				if (locals.ticket.winnerWeight > 0)
				{
					++locals.summary.winningTicketCount;
				}
				if (locals.ticket.status == ETicketStatus::PAID)
				{
					++locals.summary.paidTicketCount;
				}
				if (locals.ticket.bonusQualified)
				{
					++locals.summary.bonusQualifiedTicketCount;
				}
				output.players.set(locals.i, locals.summary);
				break;
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
		output.walletCreationFee = state.get().walletCreationFee;
		output.ticketCount = state.get().ticketCount;
		output.resultCounter = state.get().resultCounter;
		output.activeGameCount = state.get().activeGameCount;
		output.walletCount = static_cast<uint16>(state.get().wallets.population());
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
	PUBLIC_PROCEDURE_WITH_LOCALS(SetPlatformConfig)
	{
		if (qpi.invocationReward() != 0)
		{
			locals.refundInput.returnCode = EReturnCode::INVALID_VALUE;
			CALL(RefundInvocationReward, locals.refundInput, locals.refundOutput);
			output.returnCode = locals.refundOutput.returnCode;
			return;
		}
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
		if (input.roundFee == 0 || input.roundFee > PLDT_MAX_TRANSFER_AMOUNT || input.walletCreationFee == 0 ||
		    input.walletCreationFee > PLDT_MAX_TRANSFER_AMOUNT)
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
		state.mut().walletCreationFee = input.walletCreationFee;
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
		if (qpi.invocationReward() != 0)
		{
			locals.refundInput.returnCode = EReturnCode::INVALID_VALUE;
			CALL(RefundInvocationReward, locals.refundInput, locals.refundOutput);
			output.returnCode = locals.refundOutput.returnCode;
			return;
		}
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
		locals.purchaseInput.purchase = input;
		locals.purchaseInput.validateDigitsBeforePlayerLimit = false;
		CALL(ExecuteTicketPurchase, locals.purchaseInput, locals.purchaseOutput);
		output.ticketIds = locals.purchaseOutput.ticketIds;
		output.ticketIndexes = locals.purchaseOutput.ticketIndexes;
		output.acceptedCount = locals.purchaseOutput.acceptedCount;
		output.returnCode = locals.purchaseOutput.returnCode;
	}

	/**
	 * @brief Releases caller-owned managed shares to another managing contract.
	 * @param input Asset, share count, and destination managing contract.
	 * @param output Release fee result and status code.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(TransferShareManagementRights)
	{
		if (input.numberOfShares <= 0 || input.newManagingContractIndex == SELF_INDEX)
		{
			if (qpi.invocationReward() > 0)
			{
				locals.refundResult = qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.returnCode = locals.refundResult < 0 ? EReturnCode::TRANSFER_FAILED : EReturnCode::INVALID_VALUE;
			return;
		}
		locals.possessedShares =
		    qpi.numberOfPossessedShares(input.asset.assetName, input.asset.issuer, qpi.invocator(), qpi.invocator(), SELF_INDEX, SELF_INDEX);
		if (locals.possessedShares < input.numberOfShares)
		{
			if (qpi.invocationReward() > 0)
			{
				locals.refundResult = qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.returnCode = locals.refundResult < 0 ? EReturnCode::TRANSFER_FAILED : EReturnCode::INSUFFICIENT_FUNDS;
			return;
		}
		locals.walletFound = state.get().wallets.get(qpi.invocator(), locals.wallet);
		if (locals.walletFound)
		{
			if (locals.wallet.status != EWalletStatus::OPEN && locals.wallet.status != EWalletStatus::EXPIRING)
			{
				if (qpi.invocationReward() > 0)
				{
					locals.refundResult = qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				output.returnCode = locals.refundResult < 0 ? EReturnCode::TRANSFER_FAILED : EReturnCode::INVALID_STATE;
				return;
			}
			locals.walletAssetFound = findWalletAsset(locals.wallet, input.asset, locals.i, locals.walletAsset);
			if (locals.walletAssetFound)
			{
				locals.walletAssetSlot = static_cast<uint8>(locals.i);
			}
		}
		output.transferResult = qpi.releaseShares(input.asset, qpi.invocator(), qpi.invocator(), input.numberOfShares, input.newManagingContractIndex,
		                                          input.newManagingContractIndex, qpi.invocationReward());
		if (output.transferResult < 0)
		{
			if (qpi.invocationReward() > 0)
			{
				locals.refundResult = qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.returnCode = EReturnCode::TRANSFER_FAILED;
			return;
		}
		if (locals.walletFound && locals.walletAssetFound)
		{
			locals.walletDebit = locals.walletAsset.balance < static_cast<uint64>(input.numberOfShares) ? locals.walletAsset.balance
			                                                                                            : static_cast<uint64>(input.numberOfShares);
			locals.walletAsset.balance -= locals.walletDebit;
			if (locals.walletAsset.balance == 0 && locals.walletAsset.activeGameReferences == 0)
			{
				setMemory(locals.walletAsset, 0);
				--locals.wallet.assetCount;
			}
			locals.wallet.assets.set(locals.walletAssetSlot, locals.walletAsset);
			state.mut().wallets.replace(qpi.invocator(), locals.wallet);
		}
		locals.refundAmount = qpi.invocationReward() - output.transferResult;
		if (locals.refundAmount > 0)
		{
			locals.refundResult = qpi.transfer(qpi.invocator(), locals.refundAmount);
			if (locals.refundResult < 0)
			{
				output.returnCode = EReturnCode::TRANSFER_FAILED;
				return;
			}
		}
		output.returnCode = EReturnCode::SUCCESS;
	}

	/**
	 * @brief Pays platform accruals for one managed asset accounting bucket.
	 * @param input Asset issuance and its managing contract indexes.
	 * @param output Developer and shareholder amounts paid plus result code.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(WithdrawAssetPlatformRevenue)
	{
		if (qpi.invocationReward() != 0)
		{
			locals.refundInput.returnCode = EReturnCode::INVALID_VALUE;
			CALL(RefundInvocationReward, locals.refundInput, locals.refundOutput);
			output.returnCode = locals.refundOutput.returnCode;
			return;
		}
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
			if (locals.accounting.isActive && isSameAsset(locals.accounting.asset, input.asset) &&
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
		// An in-flight dividend owns its accounting bucket until every snapshotted entitlement is paid.
		if (state.get().assetDividendDistribution.active &&
		    (!isSameAsset(state.get().assetDividendDistribution.asset, input.asset) ||
		     state.get().assetDividendDistribution.accountingSlot != static_cast<uint16>(locals.i)))
		{
			output.returnCode = EReturnCode::INVALID_STATE;
			return;
		}
		// A resumed dividend has priority over newer accruals so later traffic cannot starve its unpaid recipients.
		if (!state.get().assetDividendDistribution.active && locals.accounting.developer1Accrued > 0)
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
					locals.creditInput.owner = state.get().developer1;
					locals.creditInput.asset = locals.accounting.asset;
					locals.creditInput.amount = locals.accounting.developer1Accrued;
					CALL(CreditWalletAsset, locals.creditInput, locals.creditOutput);
					output.developer1Paid = locals.accounting.developer1Accrued;
					locals.accounting.developer1Accrued = 0;
				}
				else
				{
					locals.failed = true;
				}
			}
		}
		if (!state.get().assetDividendDistribution.active && locals.accounting.developer2Accrued > 0)
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
					locals.creditInput.owner = state.get().developer2;
					locals.creditInput.asset = locals.accounting.asset;
					locals.creditInput.amount = locals.accounting.developer2Accrued;
					CALL(CreditWalletAsset, locals.creditInput, locals.creditOutput);
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
		locals.dividendInput.accountingSlot = static_cast<uint16>(locals.i);
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
	PRIVATE_FUNCTION_WITH_LOCALS(ValidateGameConfiguration)
	{
		output.returnCode = EReturnCode::INVALID_VALUE;
		locals.managedCurrencyShares = 0;
		locals.maxDrawAt = qpi.now();
		if (!locals.maxDrawAt.addDays(PLDT_MAX_SCHEDULE_DAYS))
		{
			return;
		}
		if (input.configuration.currencyMode == ECurrencyMode::ASSET)
		{
			locals.managedCurrencyShares = qpi.numberOfShares(input.configuration.currencyAsset, AssetOwnershipSelect::byManagingContract(SELF_INDEX),
			                                                  AssetPossessionSelect::byManagingContract(SELF_INDEX));
		}
		if (input.configuration.initialRunCredit < state.get().roundFee ||
		    input.configuration.initialCreatorBalance < input.configuration.creatorPrizeSeed)
		{
			output.returnCode = EReturnCode::INSUFFICIENT_FUNDS;
			return;
		}
		if (!input.configuration.startAt.isValid() || !input.configuration.drawAt.isValid() || input.configuration.startAt <= qpi.now() ||
		    input.configuration.drawAt > locals.maxDrawAt || input.configuration.drawAt <= input.configuration.startAt)
		{
			return;
		}
		if (input.configuration.ticketPrice == 0 || input.configuration.ticketPrice > PLDT_MAX_TRANSFER_AMOUNT ||
		    input.configuration.creatorPrizeSeed > PLDT_MAX_TRANSFER_AMOUNT || input.configuration.initialRunCredit > PLDT_MAX_TRANSFER_AMOUNT ||
		    input.configuration.initialCreatorBalance > PLDT_MAX_TRANSFER_AMOUNT ||
		    (input.configuration.currencyMode == ECurrencyMode::QUBIC &&
		     input.configuration.initialRunCredit > PLDT_MAX_TRANSFER_AMOUNT - input.configuration.initialCreatorBalance))
		{
			return;
		}
		if (input.configuration.ticketLimit == 0 || input.configuration.ticketLimit > PLDT_MAX_TICKETS_PER_GAME ||
		    input.configuration.playerTicketLimit == 0 || input.configuration.playerTicketLimit > input.configuration.ticketLimit ||
		    input.configuration.codeLength < PLDT_MIN_CODE_LENGTH || input.configuration.codeLength > PLDT_MAX_CODE_LENGTH ||
		    input.configuration.maxDigit < PLDT_MIN_MAX_DIGIT || input.configuration.maxDigit > PLDT_MAX_DIGIT ||
		    (!input.configuration.allowRepeatedDigits && input.configuration.maxDigit + 1 < input.configuration.codeLength))
		{
			return;
		}
		if (input.configuration.creatorFeePercent > state.get().maxCreatorFeePercent ||
		    input.configuration.creatorFeePercent > PLDT_MAX_CREATOR_FEE_PERCENT || input.configuration.bonusAssetCount > PLDT_MAX_BONUS_ASSETS)
		{
			return;
		}
		if ((input.configuration.mode != EGameMode::ONE_SHOT && input.configuration.mode != EGameMode::PERMANENT) ||
		    (input.configuration.currencyMode != ECurrencyMode::QUBIC && input.configuration.currencyMode != ECurrencyMode::ASSET))
		{
			return;
		}
		if ((input.configuration.bonusAssetCount > 0 && (input.configuration.bonusMultiplierBps < PLDT_BONUS_MULTIPLIER_SCALE ||
		                                                 input.configuration.bonusMultiplierBps > PLDT_MAX_BONUS_MULTIPLIER_BPS)) ||
		    (input.configuration.currencyMode == ECurrencyMode::ASSET &&
		     (input.configuration.currencyAsset.assetName == 0 ||
		      !qpi.isAssetIssued(input.configuration.currencyAsset.issuer, input.configuration.currencyAsset.assetName) ||
		      locals.managedCurrencyShares <= 0 || input.configuration.ownershipManagingContractIndex != SELF_INDEX ||
		      input.configuration.possessionManagingContractIndex != SELF_INDEX)))
		{
			return;
		}
		for (locals.i = 0; locals.i < input.configuration.bonusAssetCount; ++locals.i)
		{
			if (input.configuration.bonusAssets.get(locals.i).assetName == 0 ||
			    !qpi.isAssetIssued(input.configuration.bonusAssets.get(locals.i).issuer, input.configuration.bonusAssets.get(locals.i).assetName))
			{
				return;
			}
		}
		output.returnCode = EReturnCode::SUCCESS;
	}

	PRIVATE_FUNCTION_WITH_LOCALS(ValidateTierConfiguration)
	{
		output.returnCode = EReturnCode::INVALID_VALUE;
		locals.weightTotal = 0;
		for (locals.exact = 0; locals.exact <= PLDT_MAX_CODE_LENGTH; ++locals.exact)
		{
			for (locals.misplaced = 0; locals.misplaced <= PLDT_MAX_CODE_LENGTH - locals.exact; ++locals.misplaced)
			{
				locals.index = payoutMatrixIndex(locals.exact, locals.misplaced);
				locals.weightTotal = sadd(locals.weightTotal, static_cast<uint64>(input.tierWeightsBps.get(static_cast<uint16>(locals.index))));
				if (input.tierWeightsBps.get(static_cast<uint16>(locals.index)) > 0 &&
				    (locals.exact > input.codeLength || locals.misplaced > input.codeLength - locals.exact ||
				     (locals.exact + 1 == input.codeLength && locals.misplaced == 1)))
				{
					return;
				}
			}
		}
		if (locals.weightTotal == PLDT_TIER_BPS_SCALE)
		{
			output.returnCode = EReturnCode::SUCCESS;
		}
	}

	PRIVATE_FUNCTION_WITH_LOCALS(CalculateGamePreview)
	{
		output.preview.returnCode = EReturnCode::INVALID_VALUE;
		locals.economicsInput.ticketPrice = input.configuration.ticketPrice;
		locals.economicsInput.creatorFeePercent = input.configuration.creatorFeePercent;
		locals.economicsInput.platformFeePercent = state.get().platformFeePercent;
		calculateTicketEconomics(locals.economicsInput, locals.economicsOutput);
		if (input.configuration.currencyMode == ECurrencyMode::ASSET && input.configuration.currencyAsset.issuer == NULL_ID &&
		    locals.economicsOutput.burn > 0)
		{
			return;
		}
		locals.refundablePerTicket = sadd(locals.economicsOutput.creatorFee, locals.economicsOutput.prizeContribution);
		if (input.configuration.ticketPrice > div(PLDT_MAX_TRANSFER_AMOUNT, static_cast<uint64>(input.configuration.ticketLimit)) ||
		    locals.refundablePerTicket >
		        div(PLDT_MAX_TRANSFER_AMOUNT - input.configuration.creatorPrizeSeed, static_cast<uint64>(input.configuration.ticketLimit)))
		{
			return;
		}
		output.preview.platformFee = locals.economicsOutput.platformFee;
		output.preview.roundFee = state.get().roundFee;
		output.preview.operationFee = PLDT_OPERATION_FEE;
		if (input.configuration.currencyMode == ECurrencyMode::QUBIC)
		{
			output.preview.initialQubicRequired = sadd(input.configuration.initialRunCredit, input.configuration.initialCreatorBalance);
		}
		else
		{
			output.preview.initialQubicRequired = input.configuration.initialRunCredit;
			output.preview.initialCreatorAssetRequired = input.configuration.initialCreatorBalance;
		}
		output.preview.creatorFee = locals.economicsOutput.creatorFee;
		output.preview.burn = locals.economicsOutput.burn;
		output.preview.prizeContribution = locals.economicsOutput.prizeContribution;
		output.preview.returnCode = EReturnCode::SUCCESS;
	}

	PRIVATE_FUNCTION_WITH_LOCALS(PrepareGameCreation)
	{
		output.returnCode = EReturnCode::INVALID_VALUE;
		locals.maxDrawAt = qpi.now();
		if (input.configuration.startAt <= qpi.now() || !locals.maxDrawAt.addDays(PLDT_MAX_SCHEDULE_DAYS) ||
		    input.configuration.drawAt > locals.maxDrawAt)
		{
			return;
		}
		if (qpi.invocationReward() != 0)
		{
			output.returnCode = EReturnCode::TICKET_INVALID_PRICE;
			return;
		}
		if (!state.get().wallets.get(qpi.invocator(), locals.wallet) || locals.wallet.status != EWalletStatus::OPEN)
		{
			output.returnCode = EReturnCode::INVALID_STATE;
			return;
		}
		// Reserve the exact wallet buckets and platform accrual capacity without mutating state.
		output.serviceCreditDebit =
		    locals.wallet.serviceCredit < input.configuration.initialRunCredit ? locals.wallet.serviceCredit : input.configuration.initialRunCredit;
		output.refundableQubicDebit = input.configuration.initialRunCredit - output.serviceCreditDebit;
		if (input.configuration.currencyMode == ECurrencyMode::QUBIC)
		{
			output.refundableQubicDebit = sadd(output.refundableQubicDebit, input.configuration.initialCreatorBalance);
		}
		if (output.refundableQubicDebit > locals.wallet.refundableQubic)
		{
			output.returnCode = EReturnCode::INSUFFICIENT_FUNDS;
			return;
		}
		calculatePlatformShares(state.get().roundFee, output.developer1Fee, output.developer2Fee, output.dividendFee);
		if (!hasPlatformAccrualCapacity(state.get().developer1Accrued, state.get().developer2Accrued, state.get().dividendAccrued,
		                                output.developer1Fee, output.developer2Fee, output.dividendFee))
		{
			output.returnCode = EReturnCode::STORAGE_FULL;
			return;
		}
		// Invariant: cleared slots are linked once; never-used slots form a contiguous suffix.
		locals.creatorActiveGames = locals.wallet.activeGameCount;
		if (locals.creatorActiveGames >= PLDT_MAX_ACTIVE_GAMES_PER_CREATOR)
		{
			output.returnCode = EReturnCode::STORAGE_FULL;
			return;
		}
		if (state.get().freeGameHead != 0)
		{
			output.slot = state.get().freeGameHead - 1;
		}
		else if (state.get().nextUnusedGameSlot < PLDT_MAX_GAMES)
		{
			output.slot = state.get().nextUnusedGameSlot;
		}
		else
		{
			output.returnCode = EReturnCode::STORAGE_FULL;
			return;
		}
		if (state.get().games.get(output.slot).status != EGameStatus::EMPTY_SLOT)
		{
			output.returnCode = EReturnCode::STORAGE_FULL;
			return;
		}
		if (input.configuration.currencyMode == ECurrencyMode::ASSET)
		{
			// Resolve wallet custody and the shared asset-accounting bucket before the commit phase.
			locals.walletAssetFound = false;
			locals.walletFreeAssetSlotFound = false;
			for (locals.i = 0; locals.i < locals.wallet.assets.capacity(); ++locals.i)
			{
				locals.walletAsset = locals.wallet.assets.get(locals.i);
				if (locals.walletAsset.isActive && isSameAsset(locals.walletAsset.asset, input.configuration.currencyAsset))
				{
					output.walletAssetSlot = static_cast<uint8>(locals.i);
					locals.walletAssetFound = true;
					break;
				}
				if (!locals.walletAsset.isActive && !locals.walletFreeAssetSlotFound)
				{
					locals.walletFreeAssetSlot = static_cast<uint8>(locals.i);
					locals.walletFreeAssetSlotFound = true;
				}
			}
			if (!locals.walletAssetFound && input.configuration.initialCreatorBalance == 0 && locals.walletFreeAssetSlotFound &&
			    locals.wallet.assetCount < PLDT_MAX_WALLET_ASSETS)
			{
				output.walletAssetSlot = locals.walletFreeAssetSlot;
			}
			else if (!locals.walletAssetFound || locals.walletAsset.balance < input.configuration.initialCreatorBalance)
			{
				output.returnCode = EReturnCode::INSUFFICIENT_FUNDS;
				return;
			}
			locals.accountingFound = false;
			for (locals.i = 0; locals.i < state.get().assetAccounting.capacity(); ++locals.i)
			{
				locals.assetAccounting = state.get().assetAccounting.get(locals.i);
				if (locals.assetAccounting.isActive && isSameAsset(locals.assetAccounting.asset, input.configuration.currencyAsset) &&
				    locals.assetAccounting.ownershipManagingContractIndex == input.configuration.ownershipManagingContractIndex &&
				    locals.assetAccounting.possessionManagingContractIndex == input.configuration.possessionManagingContractIndex)
				{
					output.accountingSlot = static_cast<uint16>(locals.i);
					output.assetAccounting = locals.assetAccounting;
					locals.accountingFound = true;
					break;
				}
			}
			for (locals.i = 0; !locals.accountingFound && locals.i < state.get().assetAccounting.capacity(); ++locals.i)
			{
				locals.assetAccounting = state.get().assetAccounting.get(locals.i);
				if (!locals.assetAccounting.isActive)
				{
					output.accountingSlot = static_cast<uint16>(locals.i);
					output.assetAccounting.asset = input.configuration.currencyAsset;
					output.assetAccounting.ownershipManagingContractIndex = input.configuration.ownershipManagingContractIndex;
					output.assetAccounting.possessionManagingContractIndex = input.configuration.possessionManagingContractIndex;
					output.assetAccounting.isActive = true;
					locals.accountingFound = true;
				}
			}
			if (!locals.accountingFound)
			{
				output.returnCode = EReturnCode::STORAGE_FULL;
				return;
			}
		}
		output.returnCode = EReturnCode::SUCCESS;
	}

	PRIVATE_PROCEDURE_WITH_LOCALS(CollectInitialAssetFunding)
	{
		output.returnCode = EReturnCode::SUCCESS;
		if (input.configuration.currencyMode != ECurrencyMode::ASSET || input.configuration.initialCreatorBalance == 0)
		{
			return;
		}
		locals.possessedShares = qpi.numberOfPossessedShares(input.configuration.currencyAsset.assetName, input.configuration.currencyAsset.issuer,
		                                                     qpi.invocator(), qpi.invocator(), input.configuration.ownershipManagingContractIndex,
		                                                     input.configuration.possessionManagingContractIndex);
		if (locals.possessedShares < static_cast<sint64>(input.configuration.initialCreatorBalance))
		{
			output.returnCode = EReturnCode::INSUFFICIENT_FUNDS;
			return;
		}
		locals.transferResult = qpi.transferShareOwnershipAndPossession(input.configuration.currencyAsset.assetName,
		                                                                input.configuration.currencyAsset.issuer, qpi.invocator(), qpi.invocator(),
		                                                                static_cast<sint64>(input.configuration.initialCreatorBalance), SELF);
		if (locals.transferResult < 0)
		{
			output.returnCode = EReturnCode::TRANSFER_FAILED;
		}
	}

	PRIVATE_PROCEDURE_WITH_LOCALS(CommitCreatedGame)
	{
		if (state.get().freeGameHead != 0)
		{
			state.mut().freeGameHead = state.get().freeGameNext.get(input.prepared.slot);
			state.mut().freeGameNext.set(input.prepared.slot, 0);
		}
		else
		{
			state.mut().nextUnusedGameSlot = input.prepared.slot + 1;
		}
		locals.generation = sadd(state.get().generations.get(input.prepared.slot), 1ULL);
		if (locals.generation == 0)
		{
			locals.generation = 1;
		}
		state.mut().generations.set(input.prepared.slot, locals.generation);
		setMemory(locals.game, 0);
		locals.game.tierWeightsBps = input.configuration.tierWeightsBps;
		locals.game.bonusAssets = input.configuration.bonusAssets;
		locals.game.name = input.configuration.name;
		locals.game.currencyAsset = input.configuration.currencyAsset;
		locals.game.owner = qpi.invocator();
		locals.game.startAt = input.configuration.startAt;
		locals.game.drawAt = input.configuration.drawAt;
		locals.game.gameId = (locals.generation << 10) | input.prepared.slot;
		locals.game.ticketPrice = input.configuration.ticketPrice;
		locals.game.creatorPrizeSeed = input.configuration.creatorPrizeSeed;
		locals.game.prizePool = input.configuration.creatorPrizeSeed;
		locals.game.roundFeeSnapshot = state.get().roundFee;
		locals.game.runCredit = input.configuration.initialRunCredit - state.get().roundFee;
		locals.game.creatorBalance = input.configuration.initialCreatorBalance - input.configuration.creatorPrizeSeed;
		locals.game.runServiceCredit =
		    input.prepared.serviceCreditDebit > state.get().roundFee ? input.prepared.serviceCreditDebit - state.get().roundFee : 0;
		locals.game.roundDurationMicroseconds = input.configuration.startAt.durationMicrosec(input.configuration.drawAt);
		locals.game.bonusMultiplierBps = input.configuration.bonusMultiplierBps;
		locals.game.ticketLimit = input.configuration.ticketLimit;
		locals.game.playerTicketLimit = input.configuration.playerTicketLimit;
		locals.game.bonusAssetCount = input.configuration.bonusAssetCount;
		locals.game.ownershipManagingContractIndex = input.configuration.ownershipManagingContractIndex;
		locals.game.possessionManagingContractIndex = input.configuration.possessionManagingContractIndex;
		locals.game.bonusOwnershipManagingContractIndex = input.configuration.bonusOwnershipManagingContractIndex;
		locals.game.bonusPossessionManagingContractIndex = input.configuration.bonusPossessionManagingContractIndex;
		locals.game.assetAccountingLink = input.configuration.currencyMode == ECurrencyMode::ASSET ? input.prepared.accountingSlot + 1 : 0;
		locals.game.codeLength = input.configuration.codeLength;
		locals.game.maxDigit = input.configuration.maxDigit;
		locals.game.creatorFeePercent = input.configuration.creatorFeePercent;
		locals.game.currencyMode = input.configuration.currencyMode;
		locals.game.mode = input.configuration.mode;
		locals.game.roundNumber = 1;
		locals.game.allowRepeatedDigits = input.configuration.allowRepeatedDigits;
		locals.game.status = EGameStatus::SCHEDULED;
		if (input.configuration.currencyMode == ECurrencyMode::ASSET)
		{
			locals.assetAccounting = input.prepared.assetAccounting;
			++locals.assetAccounting.activeGameCount;
			state.mut().assetAccounting.set(input.prepared.accountingSlot, locals.assetAccounting);
		}
		state.get().wallets.get(qpi.invocator(), locals.wallet);
		locals.wallet.serviceCredit -= input.prepared.serviceCreditDebit;
		locals.wallet.refundableQubic -= input.prepared.refundableQubicDebit;
		++locals.wallet.activeGameCount;
		locals.wallet.lastGameCreationEpoch = qpi.epoch();
		if (input.configuration.currencyMode == ECurrencyMode::ASSET)
		{
			locals.walletAsset = locals.wallet.assets.get(input.prepared.walletAssetSlot);
			if (!locals.walletAsset.isActive)
			{
				setMemory(locals.walletAsset, 0);
				locals.walletAsset.asset = input.configuration.currencyAsset;
				locals.walletAsset.isActive = true;
				++locals.wallet.assetCount;
			}
			locals.walletAsset.balance -= input.configuration.initialCreatorBalance;
			++locals.walletAsset.activeGameReferences;
			locals.wallet.assets.set(input.prepared.walletAssetSlot, locals.walletAsset);
		}
		state.mut().wallets.replace(qpi.invocator(), locals.wallet);
		state.mut().games.set(input.prepared.slot, locals.game);
		state.mut().developer1Accrued = sadd(state.get().developer1Accrued, input.prepared.developer1Fee);
		state.mut().developer2Accrued = sadd(state.get().developer2Accrued, input.prepared.developer2Fee);
		state.mut().dividendAccrued = sadd(state.get().dividendAccrued, input.prepared.dividendFee);
		state.mut().activeGameCount = state.get().activeGameCount + 1;
		output.gameId = locals.game.gameId;
		output.slot = input.prepared.slot;
		output.returnCode = EReturnCode::SUCCESS;
	}

	/** Computes one ticket's exact fee and prize split without accessing contract state. */
	static void calculateTicketEconomics(const CalculateTicketEconomics_input& input, CalculateTicketEconomics_output& output)
	{
		output.platformFee = mulDiv(input.ticketPrice, input.platformFeePercent, 100ULL);
		output.net = input.ticketPrice - output.platformFee;
		output.creatorFee = mulDiv(output.net, input.creatorFeePercent, 100ULL);
		output.burn = mulDiv(output.net, PLDT_BURN_PERCENT, 100ULL);
		output.prizeContribution = output.net - output.creatorFee - output.burn;
		calculatePlatformShares(output.platformFee, output.developer1Fee, output.developer2Fee, output.dividendFee);
	}

	PRIVATE_FUNCTION_WITH_LOCALS(PrepareTicketPurchase)
	{
		output.returnCode = EReturnCode::INVALID_GAME;
		if (!isGameIdValid(state, input.purchase.gameId))
		{
			return;
		}
		if (input.purchase.ticketCount == 0 || input.purchase.ticketCount > PLDT_MAX_BATCH_TICKETS)
		{
			output.returnCode = EReturnCode::INVALID_VALUE;
			return;
		}
		output.slot = gameSlot(input.purchase.gameId);
		output.game = state.get().games.get(output.slot);
		locals.now = qpi.now();
		locals.lifecycleInput.game = output.game;
		locals.lifecycleInput.now = locals.now;
		evaluateGameLifecycle(locals.lifecycleInput, locals.lifecycleOutput);
		if (locals.lifecycleOutput.purchaseReturnCode != EReturnCode::SUCCESS)
		{
			output.returnCode = locals.lifecycleOutput.purchaseReturnCode;
			return;
		}
		output.game.status = locals.lifecycleOutput.effectiveStatus;
		if (output.game.ticketCount + input.purchase.ticketCount > output.game.ticketLimit ||
		    input.purchase.ticketCount > state.get().freeTicketCount + state.get().tickets.capacity() -
		                                     (state.get().nextUnusedTicketSlot != 0 ? state.get().nextUnusedTicketSlot : state.get().ticketCount))
		{
			output.returnCode = EReturnCode::TICKET_SOLD_OUT;
			return;
		}
		locals.validateInput.codeLength = output.game.codeLength;
		locals.validateInput.maxDigit = output.game.maxDigit;
		locals.validateInput.allowRepeatedDigits = output.game.allowRepeatedDigits;
		if (input.validateDigitsBeforePlayerLimit)
		{
			locals.validateInput.digits = input.purchase.tickets.get(0);
			CALL(ValidateDigits, locals.validateInput, locals.validateOutput);
			if (locals.validateOutput.returnCode != EReturnCode::SUCCESS)
			{
				output.returnCode = locals.validateOutput.returnCode;
				return;
			}
		}
		locals.link = output.game.firstTicketLink;
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
		if (locals.playerTickets + input.purchase.ticketCount > output.game.playerTicketLimit)
		{
			output.returnCode = EReturnCode::PLAYER_TICKET_LIMIT;
			return;
		}
		if (!input.validateDigitsBeforePlayerLimit)
		{
			for (locals.i = 0; locals.i < input.purchase.ticketCount; ++locals.i)
			{
				locals.validateInput.digits = input.purchase.tickets.get(locals.i);
				CALL(ValidateDigits, locals.validateInput, locals.validateOutput);
				if (locals.validateOutput.returnCode != EReturnCode::SUCCESS)
				{
					output.returnCode = locals.validateOutput.returnCode;
					return;
				}
			}
		}
		locals.bonusInput.game = output.game;
		locals.bonusInput.player = qpi.invocator();
		CALL(EvaluateBonusQualification, locals.bonusInput, locals.bonusOutput);
		output.bonusQualified = locals.bonusOutput.qualified;
		output.totalPrice = smul(output.game.ticketPrice, static_cast<uint64>(input.purchase.ticketCount));
		if (output.totalPrice > PLDT_MAX_TRANSFER_AMOUNT)
		{
			output.returnCode = EReturnCode::INVALID_VALUE;
			return;
		}
		locals.economicsInput.ticketPrice = output.game.ticketPrice;
		locals.economicsInput.creatorFeePercent = output.game.creatorFeePercent;
		locals.economicsInput.platformFeePercent = state.get().platformFeePercent;
		calculateTicketEconomics(locals.economicsInput, output.economics);
		if (output.game.totalRevenue > PLDT_MAX_TRANSFER_AMOUNT - output.totalPrice ||
		    output.game.creatorBalance > PLDT_MAX_TRANSFER_AMOUNT - output.game.prizePool ||
		    smul(output.economics.prizeContribution, static_cast<uint64>(input.purchase.ticketCount)) >
		        PLDT_MAX_TRANSFER_AMOUNT - output.game.prizePool - output.game.creatorBalance)
		{
			output.returnCode = EReturnCode::STORAGE_FULL;
			return;
		}
		output.returnCode = EReturnCode::SUCCESS;
	}

	PRIVATE_FUNCTION_WITH_LOCALS(ValidatePurchaseAccountingCapacity)
	{
		output.refundOnFailure = true;
		output.returnCode = EReturnCode::SUCCESS;
		locals.developer1Fee = smul(input.prepared.economics.developer1Fee, static_cast<uint64>(input.ticketCount));
		locals.developer2Fee = smul(input.prepared.economics.developer2Fee, static_cast<uint64>(input.ticketCount));
		locals.dividendFee = smul(input.prepared.economics.dividendFee, static_cast<uint64>(input.ticketCount));
		if (input.prepared.game.currencyMode == ECurrencyMode::QUBIC)
		{
			if (!hasPlatformAccrualCapacity(state.get().developer1Accrued, state.get().developer2Accrued, state.get().dividendAccrued,
			                                locals.developer1Fee, locals.developer2Fee, locals.dividendFee))
			{
				output.returnCode = EReturnCode::STORAGE_FULL;
			}
			return;
		}
		locals.assetAccounting = state.get().assetAccounting.get(input.prepared.game.assetAccountingLink - 1);
		if (!hasPlatformAccrualCapacity(locals.assetAccounting.developer1Accrued, locals.assetAccounting.developer2Accrued,
		                                locals.assetAccounting.dividendAccrued, locals.developer1Fee, locals.developer2Fee, locals.dividendFee))
		{
			output.returnCode = EReturnCode::STORAGE_FULL;
			output.refundOnFailure = false;
		}
	}

	PRIVATE_PROCEDURE_WITH_LOCALS(CollectTicketPayment)
	{
		if ((input.game.currencyMode == ECurrencyMode::QUBIC && qpi.invocationReward() != input.totalPrice) ||
		    (input.game.currencyMode == ECurrencyMode::ASSET && qpi.invocationReward() != 0))
		{
			locals.refundInput.returnCode = EReturnCode::TICKET_INVALID_PRICE;
			CALL(RefundInvocationReward, locals.refundInput, locals.refundOutput);
			output.returnCode = locals.refundOutput.returnCode;
			return;
		}
		if (input.game.currencyMode == ECurrencyMode::ASSET)
		{
			// Take asset custody before burning so a failed transfer leaves every ledger unchanged.
			locals.possessedShares =
			    qpi.numberOfPossessedShares(input.game.currencyAsset.assetName, input.game.currencyAsset.issuer, qpi.invocator(), qpi.invocator(),
			                                input.game.ownershipManagingContractIndex, input.game.possessionManagingContractIndex);
			if (locals.possessedShares < static_cast<sint64>(input.totalPrice))
			{
				output.returnCode = EReturnCode::INSUFFICIENT_FUNDS;
				return;
			}
			locals.transferResult =
			    qpi.transferShareOwnershipAndPossession(input.game.currencyAsset.assetName, input.game.currencyAsset.issuer, qpi.invocator(),
			                                            qpi.invocator(), static_cast<sint64>(input.totalPrice), SELF);
			if (locals.transferResult < 0)
			{
				output.returnCode = EReturnCode::TRANSFER_FAILED;
				return;
			}
		}
		locals.burnInput.game = input.game;
		locals.burnInput.ticketCount = input.ticketCount;
		CALL(BurnCollectedTicketPayment, locals.burnInput, locals.burnOutput);
		if (locals.burnOutput.returnCode != EReturnCode::SUCCESS)
		{
			// Compensate the custody transfer because the ticket batch has not been committed yet.
			locals.transferInput.game = input.game;
			locals.transferInput.destination = qpi.invocator();
			locals.transferInput.amount = input.totalPrice;
			CALL(TransferGameCurrency, locals.transferInput, locals.transferOutput);
			output.returnCode = EReturnCode::TRANSFER_FAILED;
			return;
		}
		if (input.game.currencyMode == ECurrencyMode::ASSET)
		{
			// Wallet accounting follows successful custody and debits only the tracked portion.
			locals.walletFound = state.get().wallets.get(qpi.invocator(), locals.wallet);
			if (locals.walletFound && locals.wallet.status == EWalletStatus::OPEN)
			{
				locals.walletAssetFound = findWalletAsset(locals.wallet, input.game.currencyAsset, locals.i, locals.walletAsset);
				if (locals.walletAssetFound)
				{
					locals.walletAssetSlot = static_cast<uint8>(locals.i);
				}
				if (locals.walletAssetFound)
				{
					locals.walletDebit = locals.walletAsset.balance < input.totalPrice ? locals.walletAsset.balance : input.totalPrice;
					locals.walletAsset.balance -= locals.walletDebit;
					if (locals.walletAsset.balance == 0 && locals.walletAsset.activeGameReferences == 0)
					{
						setMemory(locals.walletAsset, 0);
						--locals.wallet.assetCount;
					}
					locals.wallet.assets.set(locals.walletAssetSlot, locals.walletAsset);
					state.mut().wallets.replace(qpi.invocator(), locals.wallet);
				}
			}
		}
		output.returnCode = EReturnCode::SUCCESS;
	}

	PRIVATE_PROCEDURE_WITH_LOCALS(CommitTicketPurchase)
	{
		state.mut().games.set(input.prepared.slot, input.prepared.game);
		for (locals.i = 0; locals.i < input.purchase.ticketCount; ++locals.i)
		{
			locals.applyInput.gameId = input.purchase.gameId;
			locals.applyInput.digits = input.purchase.tickets.get(locals.i);
			locals.applyInput.bonusQualified = input.prepared.bonusQualified;
			CALL(ApplyAcceptedTicket, locals.applyInput, locals.applyOutput);
			if (locals.applyOutput.returnCode != EReturnCode::SUCCESS)
			{
				output.returnCode = locals.applyOutput.returnCode;
				return;
			}
			output.ticketIds.set(output.acceptedCount, locals.applyOutput.ticketId);
			output.ticketIndexes.set(output.acceptedCount++, locals.applyOutput.ticketIndex);
			output.prizeContribution = locals.applyOutput.prizeContribution;
		}
		output.returnCode = EReturnCode::SUCCESS;
	}

	PRIVATE_PROCEDURE_WITH_LOCALS(ExecuteTicketPurchase)
	{
		locals.prepareInput.purchase = input.purchase;
		locals.prepareInput.validateDigitsBeforePlayerLimit = input.validateDigitsBeforePlayerLimit;
		CALL(PrepareTicketPurchase, locals.prepareInput, locals.prepareOutput);
		if (locals.prepareOutput.returnCode != EReturnCode::SUCCESS)
		{
			locals.refundInput.returnCode = locals.prepareOutput.returnCode;
			CALL(RefundInvocationReward, locals.refundInput, locals.refundOutput);
			output.returnCode = locals.refundOutput.returnCode;
			return;
		}
		locals.capacityInput.prepared = locals.prepareOutput;
		locals.capacityInput.ticketCount = input.purchase.ticketCount;
		CALL(ValidatePurchaseAccountingCapacity, locals.capacityInput, locals.capacityOutput);
		if (locals.capacityOutput.returnCode != EReturnCode::SUCCESS)
		{
			output.returnCode = locals.capacityOutput.returnCode;
			if (locals.capacityOutput.refundOnFailure)
			{
				locals.refundInput.returnCode = locals.capacityOutput.returnCode;
				CALL(RefundInvocationReward, locals.refundInput, locals.refundOutput);
				output.returnCode = locals.refundOutput.returnCode;
			}
			return;
		}
		locals.paymentInput.game = locals.prepareOutput.game;
		locals.paymentInput.totalPrice = locals.prepareOutput.totalPrice;
		locals.paymentInput.ticketCount = input.purchase.ticketCount;
		CALL(CollectTicketPayment, locals.paymentInput, locals.paymentOutput);
		if (locals.paymentOutput.returnCode != EReturnCode::SUCCESS)
		{
			output.returnCode = locals.paymentOutput.returnCode;
			return;
		}
		locals.commitInput.purchase = input.purchase;
		locals.commitInput.prepared = locals.prepareOutput;
		CALL(CommitTicketPurchase, locals.commitInput, locals.commitOutput);
		output.ticketIds = locals.commitOutput.ticketIds;
		output.ticketIndexes = locals.commitOutput.ticketIndexes;
		output.prizeContribution = locals.commitOutput.prizeContribution;
		output.acceptedCount = locals.commitOutput.acceptedCount;
		output.returnCode = locals.commitOutput.returnCode;
	}

	/** Derives the effective game status and next action without accessing contract state. */
	static void evaluateGameLifecycle(const EvaluateGameLifecycle_input& input, EvaluateGameLifecycle_output& output)
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

	PRIVATE_PROCEDURE_WITH_LOCALS(CreditWalletAsset)
	{
		output.credited = false;
		if (input.amount == 0 || input.amount > PLDT_MAX_TRANSFER_AMOUNT || !state.get().wallets.get(input.owner, locals.wallet) ||
		    locals.wallet.status != EWalletStatus::OPEN)
		{
			return;
		}

		locals.found = false;
		locals.freeSlotFound = false;
		for (locals.i = 0; locals.i < locals.wallet.assets.capacity(); ++locals.i)
		{
			locals.walletAsset = locals.wallet.assets.get(locals.i);
			if (locals.walletAsset.isActive && isSameAsset(locals.walletAsset.asset, input.asset))
			{
				if (locals.walletAsset.balance > PLDT_MAX_TRANSFER_AMOUNT - input.amount)
				{
					return;
				}

				locals.walletAsset.balance = sadd(locals.walletAsset.balance, input.amount);
				locals.wallet.assets.set(locals.i, locals.walletAsset);
				locals.found = true;
				break;
			}

			if (!locals.walletAsset.isActive && !locals.freeSlotFound)
			{
				locals.freeSlot = static_cast<uint8>(locals.i);
				locals.freeSlotFound = true;
			}
		}
		if (!locals.found)
		{
			if (!locals.freeSlotFound || locals.wallet.assetCount >= PLDT_MAX_WALLET_ASSETS)
			{
				return;
			}
			setMemory(locals.walletAsset, 0);
			locals.walletAsset.asset = input.asset;
			locals.walletAsset.balance = input.amount;
			locals.walletAsset.isActive = true;
			locals.wallet.assets.set(locals.freeSlot, locals.walletAsset);
			++locals.wallet.assetCount;
		}
		state.mut().wallets.replace(input.owner, locals.wallet);
		output.credited = true;
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
		calculateTicketEconomics(locals.economicsInput, locals.economicsOutput);
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
		calculateTicketEconomics(locals.economicsInput, locals.economicsOutput);
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

	PRIVATE_PROCEDURE_WITH_LOCALS(SnapshotAssetDividend)
	{
		output.failed = false;
		setMemory(state.mut().assetDividendDistribution, 0);
		state.mut().assetDividendDistribution.asset = input.dividendAsset;
		state.mut().assetDividendDistribution.accountingSlot = input.accountingSlot;
		state.mut().assetDividendDistribution.remainingAmount = input.dividendAmount;
		locals.shareholdersAsset.issuer = id::zero();
		locals.shareholdersAsset.assetName = PLDT_CONTRACT_ASSET_NAME;
		locals.dividendPerShare = static_cast<sint64>(div(input.dividendAmount, static_cast<uint64>(NUMBER_OF_COMPUTORS)));
		locals.remainder = mod(input.dividendAmount, static_cast<uint64>(NUMBER_OF_COMPUTORS));
		locals.shareholdersIter.begin(locals.shareholdersAsset);
		while (!locals.shareholdersIter.reachedEnd())
		{
			locals.holderShares = locals.shareholdersIter.numberOfPossessedShares();
			if (locals.holderShares > 0)
			{
				locals.holderDividend = smul(locals.holderShares, locals.dividendPerShare);
				locals.holderRemainder = static_cast<uint64>(locals.holderShares) < locals.remainder
				                             ? static_cast<uint64>(locals.holderShares)
				                             : locals.remainder;
				locals.holderDividend =
				    static_cast<sint64>(sadd(static_cast<uint64>(locals.holderDividend), locals.holderRemainder));
				locals.remainder -= locals.holderRemainder;
				if (locals.holderDividend > 0)
				{
					if (state.get().assetDividendDistribution.recipientCount >= PLDT_MAX_DIVIDEND_RECIPIENTS)
					{
						setMemory(state.mut().assetDividendDistribution, 0);
						output.failed = true;
						return;
					}
					state.mut().assetDividendDistribution.recipients.set(state.get().assetDividendDistribution.recipientCount,
					                                                           locals.shareholdersIter.possessor());
					state.mut().assetDividendDistribution.amounts.set(state.get().assetDividendDistribution.recipientCount,
					                                                        static_cast<uint64>(locals.holderDividend));
					++state.mut().assetDividendDistribution.recipientCount;
				}
			}
			locals.shareholdersIter.next();
		}
		state.mut().assetDividendDistribution.active = true;
	}

	PRIVATE_PROCEDURE_WITH_LOCALS(TransferAssetDividend)
	{
		output.distributedAmount = 0;
		output.failed = false;
		if (state.get().assetDividendDistribution.active)
		{
			if (!isSameAsset(state.get().assetDividendDistribution.asset, input.dividendAsset) ||
			    state.get().assetDividendDistribution.accountingSlot != input.accountingSlot)
			{
				output.failed = true;
				return;
			}
		}
		else
		{
			if (input.dividendAmount == 0)
			{
				return;
			}
			locals.snapshotInput = input;
			CALL(SnapshotAssetDividend, locals.snapshotInput, locals.snapshotOutput);
			if (locals.snapshotOutput.failed)
			{
				output.failed = true;
				return;
			}
		}
		while (state.get().assetDividendDistribution.cursor < state.get().assetDividendDistribution.recipientCount)
		{
			locals.transferResult = qpi.transferShareOwnershipAndPossession(
			    state.get().assetDividendDistribution.asset.assetName, state.get().assetDividendDistribution.asset.issuer, SELF, SELF,
			    static_cast<sint64>(state.get().assetDividendDistribution.amounts.get(state.get().assetDividendDistribution.cursor)),
			    state.get().assetDividendDistribution.recipients.get(state.get().assetDividendDistribution.cursor));
			if (locals.transferResult < 0)
			{
				output.failed = true;
				return;
			}
			output.distributedAmount = sadd(
			    output.distributedAmount,
			    state.get().assetDividendDistribution.amounts.get(state.get().assetDividendDistribution.cursor));
			state.mut().assetDividendDistribution.remainingAmount -=
			    state.get().assetDividendDistribution.amounts.get(state.get().assetDividendDistribution.cursor);
			locals.creditInput.owner =
			    state.get().assetDividendDistribution.recipients.get(state.get().assetDividendDistribution.cursor);
			locals.creditInput.asset = state.get().assetDividendDistribution.asset;
			locals.creditInput.amount =
			    state.get().assetDividendDistribution.amounts.get(state.get().assetDividendDistribution.cursor);
			++state.mut().assetDividendDistribution.cursor;
			CALL(CreditWalletAsset, locals.creditInput, locals.creditOutput);
		}
		if (state.get().assetDividendDistribution.remainingAmount != 0)
		{
			output.failed = true;
			return;
		}
		setMemory(state.mut().assetDividendDistribution, 0);
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

	PRIVATE_PROCEDURE_WITH_LOCALS(ClassifySettlementTickets)
	{
		output.game = input.game;
		output.progress = input.progress;
		output.budget = input.budget;
		while (output.budget > 0 && output.game.status == EGameStatus::COUNTING && output.progress.cursorLink != 0)
		{
			locals.link = output.progress.cursorLink;
			locals.ticket = state.get().tickets.get(locals.link - 1);
			locals.matchInput.playerDigits = locals.ticket.digits;
			locals.matchInput.winningDigits = output.progress.winningDigits;
			locals.matchInput.codeLength = output.game.codeLength;
			CALL(CountMatches, locals.matchInput, locals.matchOutput);
			locals.ticket.exact = locals.matchOutput.exact;
			locals.ticket.misplaced = locals.matchOutput.misplaced;
			locals.ticket.tierIndex = locals.matchOutput.tierIndex;
			if (output.game.tierWeightsBps.get(locals.ticket.tierIndex) > 0)
			{
				locals.ticket.winnerWeight = locals.ticket.bonusQualified ? output.game.bonusMultiplierBps : PLDT_BONUS_MULTIPLIER_SCALE;
				output.progress.tierWinnerCount.set(locals.ticket.tierIndex, output.progress.tierWinnerCount.get(locals.ticket.tierIndex) + 1);
				if (locals.ticket.bonusQualified)
				{
					output.progress.tierBonusCount.set(locals.ticket.tierIndex, output.progress.tierBonusCount.get(locals.ticket.tierIndex) + 1);
				}
				++output.progress.winnerCount;
			}
			else
			{
				locals.ticket.status = ETicketStatus::LOST;
			}
			output.progress.cursorLink = locals.ticket.nextLink;
			state.mut().tickets.set(locals.link - 1, locals.ticket);
			--output.budget;
		}
	}

	PRIVATE_PROCEDURE_WITH_LOCALS(AllocateSettlementPayouts)
	{
		output.game = input.game;
		output.progress = input.progress;
		output.noWinners = output.progress.winnerCount == 0;
		if (output.noWinners)
		{
			return;
		}
		output.progress.activeTierBps = 0;
		for (locals.i = 0; locals.i < PLDT_TIER_CAPACITY; ++locals.i)
		{
			if (output.progress.tierWinnerCount.get(static_cast<uint16>(locals.i)) > 0)
			{
				output.progress.activeTierBps += output.game.tierWeightsBps.get(static_cast<uint16>(locals.i));
			}
		}
		output.progress.allocatedPool = 0;
		for (locals.i = 0; locals.i < PLDT_TIER_CAPACITY; ++locals.i)
		{
			if (output.progress.tierWinnerCount.get(static_cast<uint16>(locals.i)) == 0)
			{
				continue;
			}
			locals.tierPool = mulDiv(output.progress.prizePoolSnapshot, output.game.tierWeightsBps.get(static_cast<uint16>(locals.i)),
			                         output.progress.activeTierBps);
			locals.fraction = mulMod(output.progress.prizePoolSnapshot, output.game.tierWeightsBps.get(static_cast<uint16>(locals.i)),
			                         output.progress.activeTierBps);
			output.progress.tierPools.set(static_cast<uint16>(locals.i), locals.tierPool);
			output.progress.tierFractions.set(static_cast<uint16>(locals.i), locals.fraction);
			output.progress.allocatedPool = sadd(output.progress.allocatedPool, locals.tierPool);
		}
		locals.remainder = output.progress.prizePoolSnapshot - output.progress.allocatedPool;
		while (locals.remainder > 0)
		{
			locals.foundTier = false;
			locals.bestFraction = 0;
			locals.bestTier = 0;
			for (locals.i = 0; locals.i < PLDT_TIER_CAPACITY; ++locals.i)
			{
				locals.fraction = output.progress.tierFractions.get(static_cast<uint16>(locals.i));
				if (output.progress.tierWinnerCount.get(static_cast<uint16>(locals.i)) > 0 && locals.fraction != PLDT_UINT64_SENTINEL &&
				    (!locals.foundTier || locals.fraction > locals.bestFraction))
				{
					locals.foundTier = true;
					locals.bestFraction = locals.fraction;
					locals.bestTier = static_cast<uint16>(locals.i);
				}
			}
			output.progress.tierPools.set(locals.bestTier, output.progress.tierPools.get(locals.bestTier) + 1);
			output.progress.tierFractions.set(locals.bestTier, PLDT_UINT64_SENTINEL);
			--locals.remainder;
		}
		for (locals.i = 0; locals.i < PLDT_TIER_CAPACITY; ++locals.i)
		{
			locals.count = output.progress.tierWinnerCount.get(static_cast<uint16>(locals.i));
			if (locals.count == 0)
			{
				continue;
			}
			locals.bonusCount = output.progress.tierBonusCount.get(static_cast<uint16>(locals.i));
			locals.totalWeight = smul(locals.count - locals.bonusCount, static_cast<uint64>(PLDT_BONUS_MULTIPLIER_SCALE));
			locals.totalWeight = sadd(locals.totalWeight, smul(locals.bonusCount, static_cast<uint64>(output.game.bonusMultiplierBps)));
			locals.tierPool = output.progress.tierPools.get(static_cast<uint16>(locals.i));
			output.progress.normalPayout.set(static_cast<uint16>(locals.i), mulDiv(locals.tierPool, PLDT_BONUS_MULTIPLIER_SCALE, locals.totalWeight));
			output.progress.bonusPayout.set(static_cast<uint16>(locals.i),
			                                mulDiv(locals.tierPool, output.game.bonusMultiplierBps, locals.totalWeight));
			locals.floorSum = smul(locals.count - locals.bonusCount, output.progress.normalPayout.get(static_cast<uint16>(locals.i)));
			locals.floorSum = sadd(locals.floorSum, smul(locals.bonusCount, output.progress.bonusPayout.get(static_cast<uint16>(locals.i))));
			output.progress.tierRemainder.set(static_cast<uint16>(locals.i), locals.tierPool - locals.floorSum);
		}
		output.progress.cursorLink = output.game.firstTicketLink;
		output.game.winnerCount = output.progress.winnerCount;
		output.game.status = EGameStatus::PAYING;
	}

	PRIVATE_PROCEDURE_WITH_LOCALS(PaySettlementTickets)
	{
		output.game = input.game;
		output.progress = input.progress;
		output.budget = input.budget;
		output.returnCode = EReturnCode::SUCCESS;
		while (output.budget > 0 && output.game.status == EGameStatus::PAYING && output.progress.cursorLink != 0)
		{
			locals.link = output.progress.cursorLink;
			locals.ticket = state.get().tickets.get(locals.link - 1);
			if (locals.ticket.winnerWeight == 0)
			{
				locals.ticket.status = ETicketStatus::LOST;
			}
			else
			{
				locals.payout = locals.ticket.bonusQualified ? output.progress.bonusPayout.get(locals.ticket.tierIndex)
				                                             : output.progress.normalPayout.get(locals.ticket.tierIndex);
				if (output.progress.tierRemainder.get(locals.ticket.tierIndex) > 0)
				{
					++locals.payout;
				}
				locals.transferInput.game = output.game;
				locals.transferInput.destination = locals.ticket.player;
				locals.transferInput.amount = locals.payout;
				CALL(TransferGameCurrency, locals.transferInput, locals.transferOutput);
				if (locals.transferOutput.transferResult < 0)
				{
					output.returnCode = EReturnCode::TRANSFER_FAILED;
					return;
				}
				if (output.game.currencyMode == ECurrencyMode::ASSET)
				{
					// The QPI transfer does not invoke management-right callbacks, so mirror successful payouts in open wallets.
					locals.creditInput.owner = locals.ticket.player;
					locals.creditInput.asset = output.game.currencyAsset;
					locals.creditInput.amount = locals.payout;
					CALL(CreditWalletAsset, locals.creditInput, locals.creditOutput);
				}
				locals.ticket.payout = locals.payout;
				locals.ticket.status = ETicketStatus::PAID;
				output.game.totalPaid = sadd(output.game.totalPaid, locals.payout);
				if (output.game.currencyMode == ECurrencyMode::QUBIC)
				{
				}
				if (output.progress.tierRemainder.get(locals.ticket.tierIndex) > 0)
				{
					output.progress.tierRemainder.set(locals.ticket.tierIndex, output.progress.tierRemainder.get(locals.ticket.tierIndex) - 1);
				}
			}
			output.progress.cursorLink = locals.ticket.nextLink;
			state.mut().tickets.set(locals.link - 1, locals.ticket);
			--output.budget;
		}
	}

	PRIVATE_PROCEDURE_WITH_LOCALS(AdvanceSettlement)
	{
		locals.slot = input.slot;
		locals.game = state.get().games.get(locals.slot);
		locals.progress = state.get().settlements.get(locals.slot);
		locals.budget = input.actionBudget;
		locals.classifyInput.game = locals.game;
		locals.classifyInput.progress = locals.progress;
		locals.classifyInput.budget = locals.budget;
		CALL(ClassifySettlementTickets, locals.classifyInput, locals.classifyOutput);
		locals.game = locals.classifyOutput.game;
		locals.progress = locals.classifyOutput.progress;
		locals.budget = locals.classifyOutput.budget;
		if (locals.game.status == EGameStatus::COUNTING && locals.progress.cursorLink == 0)
		{
			locals.allocateInput.game = locals.game;
			locals.allocateInput.progress = locals.progress;
			CALL(AllocateSettlementPayouts, locals.allocateInput, locals.allocateOutput);
			locals.game = locals.allocateOutput.game;
			locals.progress = locals.allocateOutput.progress;
			if (locals.allocateOutput.noWinners)
			{
				state.mut().settlements.set(locals.slot, locals.progress);
				locals.finalizeInput.slot = locals.slot;
				locals.finalizeInput.reason = EGameTerminalReason::NO_WINNERS;
				CALL(FinalizeGame, locals.finalizeInput, locals.finalizeOutput);
				output.actionsUsed = input.actionBudget - locals.budget;
				output.returnCode = locals.finalizeOutput.returnCode;
				return;
			}
		}
		locals.payInput.game = locals.game;
		locals.payInput.progress = locals.progress;
		locals.payInput.budget = locals.budget;
		CALL(PaySettlementTickets, locals.payInput, locals.payOutput);
		locals.game = locals.payOutput.game;
		locals.progress = locals.payOutput.progress;
		locals.budget = locals.payOutput.budget;
		if (locals.payOutput.returnCode != EReturnCode::SUCCESS)
		{
			state.mut().settlements.set(locals.slot, locals.progress);
			state.mut().games.set(locals.slot, locals.game);
			output.actionsUsed = input.actionBudget - locals.budget;
			output.returnCode = locals.payOutput.returnCode;
			return;
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

	PRIVATE_PROCEDURE(FreezeFinalizationState)
	{
		output.game = input.game;
		if (output.game.status == EGameStatus::FINALIZING)
		{
			return;
		}
		output.game.finalizingRoundReason = input.reason;
		output.game.finalizingStopReason = EGameStopReason::NONE;
		if (input.reason == EGameTerminalReason::NO_TICKETS || input.reason == EGameTerminalReason::NO_WINNERS ||
		    input.reason == EGameTerminalReason::OWNER_CANCELLED)
		{
			output.game.pendingCreatorBalancePayout = output.game.prizePool;
		}
		output.game.pendingCreatorCurrencyPayout = output.game.creatorRevenue;
		output.game.creatorRevenue = 0;
		if (output.game.stopRequested || input.reason == EGameTerminalReason::OWNER_CANCELLED)
		{
			output.game.finalizingStopReason = EGameStopReason::OWNER_REQUESTED;
		}
		else if (output.game.mode == EGameMode::ONE_SHOT)
		{
			output.game.finalizingStopReason = EGameStopReason::ONE_SHOT_COMPLETE;
		}
		else
		{
			if (output.game.pendingEconomics.isSet)
			{
				output.game.ticketPrice = output.game.pendingEconomics.ticketPrice;
				output.game.creatorPrizeSeed = output.game.pendingEconomics.creatorPrizeSeed;
				output.game.ticketLimit = output.game.pendingEconomics.ticketLimit;
				output.game.playerTicketLimit = output.game.pendingEconomics.playerTicketLimit;
				output.game.creatorFeePercent = output.game.pendingEconomics.creatorFeePercent;
				setMemory(output.game.pendingEconomics, 0);
			}
			if (output.game.runCredit < output.game.roundFeeSnapshot || output.game.creatorBalance < output.game.creatorPrizeSeed)
			{
				output.game.finalizingStopReason = EGameStopReason::OUT_OF_FUNDS;
			}
		}
		if (output.game.finalizingStopReason != EGameStopReason::NONE)
		{
			output.game.pendingRunCreditPayout = output.game.runCredit;
			output.game.runCredit = 0;
			output.game.pendingCreatorBalancePayout = sadd(output.game.pendingCreatorBalancePayout, output.game.creatorBalance);
			output.game.creatorBalance = 0;
		}
		output.game.status = EGameStatus::FINALIZING;
		state.mut().games.set(input.slot, output.game);
	}

	PRIVATE_PROCEDURE(ResolveNextRoundSchedule)
	{
		output.game = input.game;
		if (output.game.finalizingStopReason != EGameStopReason::NONE)
		{
			return;
		}
		output.nextStartAt = qpi.now();
		output.nextDrawAt = output.nextStartAt;
		if (!output.nextDrawAt.addMicrosec(static_cast<sint64>(output.game.roundDurationMicroseconds)))
		{
			output.game.finalizingStopReason = EGameStopReason::SCHEDULE_EXHAUSTED;
			output.game.pendingRunCreditPayout = output.game.runCredit;
			output.game.runCredit = 0;
			output.game.pendingCreatorBalancePayout = sadd(output.game.pendingCreatorBalancePayout, output.game.creatorBalance);
			output.game.creatorBalance = 0;
			state.mut().games.set(input.slot, output.game);
		}
	}

	PRIVATE_PROCEDURE_WITH_LOCALS(DrainFinalizationPayouts)
	{
		output.game = input.game;
		if (!state.get().wallets.get(output.game.owner, locals.wallet) || locals.wallet.status != EWalletStatus::OPEN)
		{
			output.returnCode = EReturnCode::INVALID_STATE;
			return;
		}
		// Return run credit by provenance, allowing each bounded wallet ledger to make independent progress.
		locals.serviceCreditReturned = output.game.runServiceCredit < output.game.pendingRunCreditPayout
		                                   ? output.game.runServiceCredit
		                                   : output.game.pendingRunCreditPayout;
		locals.availableCredit = PLDT_MAX_TRANSFER_AMOUNT - locals.wallet.serviceCredit;
		locals.amountToCredit = locals.serviceCreditReturned < locals.availableCredit ? locals.serviceCreditReturned : locals.availableCredit;
		locals.wallet.serviceCredit = sadd(locals.wallet.serviceCredit, locals.amountToCredit);
		output.game.runServiceCredit -= locals.amountToCredit;
		output.game.pendingRunCreditPayout -= locals.amountToCredit;
		if (output.game.runServiceCredit == 0 && output.game.pendingRunCreditPayout > 0)
		{
			locals.availableCredit = PLDT_MAX_TRANSFER_AMOUNT - locals.wallet.refundableQubic;
			locals.amountToCredit =
			    output.game.pendingRunCreditPayout < locals.availableCredit ? output.game.pendingRunCreditPayout : locals.availableCredit;
			locals.wallet.refundableQubic = sadd(locals.wallet.refundableQubic, locals.amountToCredit);
			output.game.pendingRunCreditPayout -= locals.amountToCredit;
		}
		state.mut().wallets.replace(output.game.owner, locals.wallet);
		state.mut().games.set(input.slot, output.game);

		// Keep fees separate from pool returns: their combined liability need not fit in one wallet credit.
		for (locals.payoutIndex = 0; locals.payoutIndex < 2; ++locals.payoutIndex)
		{
			locals.pendingAmount = locals.payoutIndex == 0 ? output.game.pendingCreatorCurrencyPayout : output.game.pendingCreatorBalancePayout;
			if (locals.pendingAmount == 0)
			{
				continue;
			}
			if (output.game.currencyMode == ECurrencyMode::QUBIC)
			{
				locals.availableCredit = PLDT_MAX_TRANSFER_AMOUNT - locals.wallet.refundableQubic;
			}
			else
			{
				// Active asset games pin their wallet position until every creator liability is credited.
				locals.walletAssetFound = findWalletAsset(locals.wallet, output.game.currencyAsset, locals.i, locals.walletAsset);
				if (!locals.walletAssetFound)
				{
					output.returnCode = EReturnCode::STORAGE_FULL;
					return;
				}
				locals.availableCredit = PLDT_MAX_TRANSFER_AMOUNT - locals.walletAsset.balance;
			}
			locals.amountToCredit = locals.pendingAmount < locals.availableCredit ? locals.pendingAmount : locals.availableCredit;
			if (locals.amountToCredit == 0)
			{
				continue;
			}
			if (output.game.currencyMode == ECurrencyMode::QUBIC)
			{
				locals.wallet.refundableQubic = sadd(locals.wallet.refundableQubic, locals.amountToCredit);
			}
			else
			{
				locals.transferResult = qpi.transferShareOwnershipAndPossession(
				    output.game.currencyAsset.assetName, output.game.currencyAsset.issuer, SELF, SELF,
				    static_cast<sint64>(locals.amountToCredit), output.game.owner);
				if (locals.transferResult < 0)
				{
					output.returnCode = EReturnCode::TRANSFER_FAILED;
					return;
				}
				locals.walletAsset.balance = sadd(locals.walletAsset.balance, locals.amountToCredit);
				locals.wallet.assets.set(static_cast<uint8>(locals.i), locals.walletAsset);
			}
			if (locals.payoutIndex == 0)
			{
				output.game.pendingCreatorCurrencyPayout -= locals.amountToCredit;
			}
			else
			{
				output.game.pendingCreatorBalancePayout -= locals.amountToCredit;
			}
			// Commit each completed credit before another transfer can fail; retries consume only the remainder.
			state.mut().wallets.replace(output.game.owner, locals.wallet);
			state.mut().games.set(input.slot, output.game);
		}
		output.returnCode = output.game.pendingRunCreditPayout == 0 && output.game.pendingCreatorBalancePayout == 0 &&
		                            output.game.pendingCreatorCurrencyPayout == 0
		                        ? EReturnCode::SUCCESS
		                        : EReturnCode::STORAGE_FULL;
	}

	PRIVATE_FUNCTION(ValidateRoundFeeCapacity)
	{
		output.returnCode = EReturnCode::SUCCESS;
		if (input.game.finalizingStopReason != EGameStopReason::NONE)
		{
			return;
		}
		calculatePlatformShares(input.game.roundFeeSnapshot, output.developer1Fee, output.developer2Fee, output.dividendFee);
		if (!hasPlatformAccrualCapacity(state.get().developer1Accrued, state.get().developer2Accrued, state.get().dividendAccrued,
		                                output.developer1Fee, output.developer2Fee, output.dividendFee))
		{
			output.returnCode = EReturnCode::STORAGE_FULL;
		}
	}

	PRIVATE_FUNCTION(BuildRoundResult)
	{
		setMemory(output.result, 0);
		output.result.tierWeightsBps = input.game.tierWeightsBps;
		output.result.winningDigits = input.progress.winningDigits;
		output.result.currencyAsset = input.game.currencyAsset;
		output.result.owner = input.game.owner;
		output.result.startAt = input.game.startAt;
		output.result.drawAt = input.game.drawAt;
		output.result.gameId = input.game.gameId;
		output.result.roundNumber = input.game.roundNumber;
		output.result.resultSequence = state.get().resultCounter + 1;
		output.result.prizePool = input.game.prizePool;
		output.result.totalPaid = input.game.totalPaid;
		output.result.firstTicketLink = input.game.firstTicketLink;
		output.result.settledTick = qpi.tick();
		output.result.ticketCount = input.game.ticketCount;
		output.result.winnerCount = input.game.winnerCount;
		output.result.codeLength = input.game.codeLength;
		output.result.currencyMode = input.game.currencyMode;
		output.result.mode = input.game.mode;
		output.result.terminalReason = input.game.finalizingRoundReason;
		output.result.detailsAvailable = true;
		output.result.gameStopReason = input.game.finalizingStopReason;
	}

	PRIVATE_PROCEDURE_WITH_LOCALS(CommitFinalizedRound)
	{
		locals.resultIndex = mod(state.get().resultCounter, static_cast<uint64>(PLDT_RESULT_STORAGE_SIZE));
		state.mut().results.set(locals.resultIndex, input.result);
		state.mut().resultCounter = state.get().resultCounter + 1;
		if (input.result.gameStopReason != EGameStopReason::NONE)
		{
			locals.clearInput.slot = input.slot;
			CALL(ClearGameSlot, locals.clearInput, locals.clearOutput);
			output.returnCode = EReturnCode::SUCCESS;
			return;
		}
		locals.game = input.game;
		locals.game.runCredit -= locals.game.roundFeeSnapshot;
		locals.game.runServiceCredit -=
		    locals.game.runServiceCredit < locals.game.roundFeeSnapshot ? locals.game.runServiceCredit : locals.game.roundFeeSnapshot;
		locals.game.creatorBalance -= locals.game.creatorPrizeSeed;
		state.mut().developer1Accrued = sadd(state.get().developer1Accrued, input.fees.developer1Fee);
		state.mut().developer2Accrued = sadd(state.get().developer2Accrued, input.fees.developer2Fee);
		state.mut().dividendAccrued = sadd(state.get().dividendAccrued, input.fees.dividendFee);
		++locals.game.roundNumber;
		locals.game.startAt = input.nextStartAt;
		locals.game.drawAt = input.nextDrawAt;
		locals.game.prizePool = locals.game.creatorPrizeSeed;
		locals.game.totalRevenue = 0;
		locals.game.totalPaid = 0;
		locals.game.firstTicketLink = 0;
		locals.game.lastTicketLink = 0;
		locals.game.ticketCount = 0;
		locals.game.winnerCount = 0;
		locals.game.status = EGameStatus::SELLING;
		locals.progress = input.progress;
		setMemory(locals.progress, 0);
		state.mut().settlements.set(input.slot, locals.progress);
		state.mut().games.set(input.slot, locals.game);
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
		locals.freezeInput.game = locals.game;
		locals.freezeInput.slot = input.slot;
		locals.freezeInput.reason = input.reason;
		CALL(FreezeFinalizationState, locals.freezeInput, locals.freezeOutput);
		locals.game = locals.freezeOutput.game;
		locals.scheduleInput.game = locals.game;
		locals.scheduleInput.slot = input.slot;
		CALL(ResolveNextRoundSchedule, locals.scheduleInput, locals.scheduleOutput);
		locals.game = locals.scheduleOutput.game;
		locals.payoutsInput.game = locals.game;
		locals.payoutsInput.slot = input.slot;
		CALL(DrainFinalizationPayouts, locals.payoutsInput, locals.payoutsOutput);
		locals.game = locals.payoutsOutput.game;
		if (locals.payoutsOutput.returnCode != EReturnCode::SUCCESS)
		{
			output.returnCode = locals.payoutsOutput.returnCode;
			return;
		}
		locals.feeInput.game = locals.game;
		CALL(ValidateRoundFeeCapacity, locals.feeInput, locals.feeOutput);
		if (locals.feeOutput.returnCode != EReturnCode::SUCCESS)
		{
			output.returnCode = locals.feeOutput.returnCode;
			return;
		}
		locals.resultInput.game = locals.game;
		locals.resultInput.progress = locals.progress;
		CALL(BuildRoundResult, locals.resultInput, locals.resultOutput);
		locals.commitInput.game = locals.game;
		locals.commitInput.progress = locals.progress;
		locals.commitInput.result = locals.resultOutput.result;
		locals.commitInput.nextStartAt = locals.scheduleOutput.nextStartAt;
		locals.commitInput.nextDrawAt = locals.scheduleOutput.nextDrawAt;
		locals.commitInput.fees = locals.feeOutput;
		locals.commitInput.slot = input.slot;
		CALL(CommitFinalizedRound, locals.commitInput, locals.commitOutput);
		output.returnCode = locals.commitOutput.returnCode;
	}

	PRIVATE_PROCEDURE_WITH_LOCALS(ClearGameSlot)
	{
		locals.game = state.get().games.get(input.slot);
		if (state.get().wallets.get(locals.game.owner, locals.wallet))
		{
			if (locals.wallet.activeGameCount > 0)
			{
				--locals.wallet.activeGameCount;
			}
			if (locals.wallet.activeGameCount == 0)
			{
				locals.wallet.serviceCreditUnlocked = true;
			}
			if (locals.game.currencyMode == ECurrencyMode::ASSET)
			{
				locals.walletAssetFound = findWalletAsset(locals.wallet, locals.game.currencyAsset, locals.i, locals.walletAsset);
				if (locals.walletAssetFound)
				{
					locals.walletAssetSlot = static_cast<uint8>(locals.i);
				}
				if (locals.walletAssetFound)
				{
					if (locals.walletAsset.activeGameReferences > 0)
					{
						--locals.walletAsset.activeGameReferences;
					}
					if (locals.walletAsset.activeGameReferences == 0 && locals.walletAsset.balance == 0)
					{
						setMemory(locals.walletAsset, 0);
						--locals.wallet.assetCount;
					}
					locals.wallet.assets.set(locals.walletAssetSlot, locals.walletAsset);
				}
			}
			state.mut().wallets.replace(locals.game.owner, locals.wallet);
		}
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
		state.mut().freeGameNext.set(input.slot, state.get().freeGameHead);
		state.mut().freeGameHead = input.slot + 1;
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
		evaluateGameLifecycle(locals.lifecycleInput, locals.lifecycleOutput);
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

	/** Returns whether two asset descriptors identify the same issuance. */
	static bool isSameAsset(const Asset& left, const Asset& right) { return left.assetName == right.assetName && left.issuer == right.issuer; }

	/** Finds an active wallet asset and returns its slot and value through caller-owned storage. */
	static bool findWalletAsset(const CreatorWallet& wallet, const Asset& asset, uint64& index, WalletAssetBalance& walletAsset)
	{
		for (index = 0; index < wallet.assets.capacity(); ++index)
		{
			walletAsset = wallet.assets.get(index);
			if (walletAsset.isActive && isSameAsset(walletAsset.asset, asset))
			{
				return true;
			}
		}
		return false;
	}

	/** Splits platform revenue while assigning integer-division remainder to dividends. */
	static void calculatePlatformShares(const uint64 total, uint64& developer1, uint64& developer2, uint64& dividend)
	{
		developer1 = mulDiv(total, PLDT_PLATFORM_DEV1_SHARE_PERCENT, 100ULL);
		developer2 = mulDiv(total, PLDT_PLATFORM_DEV2_SHARE_PERCENT, 100ULL);
		dividend = total - developer1 - developer2;
	}

	/** Checks whether three platform accrual buckets can accept their respective increments. */
	static bool hasPlatformAccrualCapacity(const uint64 developer1Accrued, const uint64 developer2Accrued, const uint64 dividendAccrued,
	                                       const uint64 developer1Increment, const uint64 developer2Increment, const uint64 dividendIncrement)
	{
		return developer1Accrued <= PLDT_MAX_TRANSFER_AMOUNT - developer1Increment &&
		       developer2Accrued <= PLDT_MAX_TRANSFER_AMOUNT - developer2Increment && dividendAccrued <= PLDT_MAX_TRANSFER_AMOUNT - dividendIncrement;
	}

	/** Debits the fixed creator-operation fee, consuming service credit before refundable Qubic. */
	static bool debitWalletOperationFee(CreatorWallet& wallet)
	{
		if (wallet.serviceCredit >= PLDT_OPERATION_FEE)
		{
			wallet.serviceCredit -= PLDT_OPERATION_FEE;
			return true;
		}
		if (wallet.refundableQubic < PLDT_OPERATION_FEE - wallet.serviceCredit)
		{
			return false;
		}
		wallet.refundableQubic -= PLDT_OPERATION_FEE - wallet.serviceCredit;
		wallet.serviceCredit = 0;
		return true;
	}

	/** Calculates the wallet buckets used for a Qubic credit and validates their capacity. */
	static bool prepareWalletQubicCredit(const CreatorWallet& wallet, const uint64 amount, const uint64 runServiceCredit,
	                                     uint64& serviceCreditReturned)
	{
		serviceCreditReturned = runServiceCredit < amount ? runServiceCredit : amount;
		return wallet.serviceCredit <= PLDT_MAX_TRANSFER_AMOUNT - serviceCreditReturned &&
		       wallet.refundableQubic <= PLDT_MAX_TRANSFER_AMOUNT - (amount - serviceCreditReturned);
	}

	/** Resolves a currently active game and verifies its owner. */
	static EReturnCode resolveOwnedGame(const QPI::ContractState<StateData, CONTRACT_INDEX>& state, const uint64 gameId, const id& owner,
	                                    uint16& slot, Game& game)
	{
		slot = gameSlot(gameId);
		if (!isGameIdValid(state, gameId))
		{
			return EReturnCode::INVALID_GAME;
		}
		game = state.get().games.get(slot);
		return game.owner == owner ? EReturnCode::SUCCESS : EReturnCode::ACCESS_DENIED;
	}

	/** Calculates an automation scan slot relative to the current cursor. */
	static constexpr uint16 automationGameSlot(const uint16 inspected, const uint16 automationCursor)
	{
		return static_cast<uint16>(mod(static_cast<uint64>(automationCursor + inspected), static_cast<uint64>(PLDT_MAX_GAMES)));
	}

	/** Returns whether a modular uint16 epoch interval is old enough without accepting future epochs. */
	static constexpr bool epochElapsedAtLeast(const uint16 current, const uint16 previous, const uint16 required)
	{
		return static_cast<uint16>(current - previous) >= required && static_cast<uint16>(current - previous) < 0x8000U;
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
