/**
 * @file PulseEditor.h
 * @brief MVP constructor for on-chain code-guessing games.
 *
 * PulseEditor supports same-currency fixed payouts for Qubic tickets or managed
 * asset-share tickets. Each template has one active round at a time.
 */

using namespace QPI;

/**
 * @brief Returns the smallest power-of-two value that is greater than or equal to `value`.
 * @param value Logical element count that must fit into a QPI `Array`.
 * @param power Current recursive candidate; callers should use the default value.
 * @return Power-of-two capacity suitable for QPI arrays.
 * @note QPI `Array` capacity must be 2^N, while product-level matrix sizes are often not powers of two.
 */
constexpr uint16 pulseEditorNextPowerOfTwo(const uint16 value, const uint16 power = 1)
{
	return power >= value ? power : pulseEditorNextPowerOfTwo(value, power << 1);
}

/// Maximum number of game templates stored by the MVP contract.
constexpr uint32 PULSEEDITOR_MAX_TEMPLATES = 1024;
/// Maximum number of tickets retained globally across stored rounds.
constexpr uint32 PULSEEDITOR_MAX_TICKETS = 1024 * PULSEEDITOR_MAX_TEMPLATES;
/// Maximum number of winner history entries retained in the ring buffer.
constexpr uint32 PULSEEDITOR_MAX_WINNERS = 1024 * PULSEEDITOR_MAX_TEMPLATES;
/// Maximum number of winner history entries returned by one `GetWinners` call.
constexpr uint16 PULSEEDITOR_WINNERS_PAGE_SIZE = 512;
static_assert((PULSEEDITOR_WINNERS_PAGE_SIZE & (PULSEEDITOR_WINNERS_PAGE_SIZE - 1)) == 0);
/// Maximum number of player tickets returned by one `GetPlayerTickets` call.
constexpr uint16 PULSEEDITOR_TICKETS_PAGE_SIZE = 256;
static_assert((PULSEEDITOR_TICKETS_PAGE_SIZE & (PULSEEDITOR_TICKETS_PAGE_SIZE - 1)) == 0);
/// Maximum number of templates returned by one discovery query.
constexpr uint16 PULSEEDITOR_TEMPLATES_PAGE_SIZE = 64;
static_assert((PULSEEDITOR_TEMPLATES_PAGE_SIZE & (PULSEEDITOR_TEMPLATES_PAGE_SIZE - 1)) == 0);
/// Maximum number of tickets accepted by one batched purchase call.
constexpr uint16 PULSEEDITOR_MAX_BATCH_TICKETS = 16;
static_assert((PULSEEDITOR_MAX_BATCH_TICKETS & (PULSEEDITOR_MAX_BATCH_TICKETS - 1)) == 0);
/// Maximum number of assets that can qualify a winner for the multiplier bonus.
constexpr uint16 PULSEEDITOR_MAX_BONUS_ASSETS = 8;
static_assert((PULSEEDITOR_MAX_BONUS_ASSETS & (PULSEEDITOR_MAX_BONUS_ASSETS - 1)) == 0);
/// Maximum supported code length; each ticket uses at most this many digits.
constexpr uint8 PULSEEDITOR_MAX_CODE_LENGTH = 10;
/// QPI-aligned digit storage capacity for ticket and result arrays.
constexpr uint8 PULSEEDITOR_DIGITS_ALIGNED = pulseEditorNextPowerOfTwo(PULSEEDITOR_MAX_CODE_LENGTH);
/// Maximum allowed digit value; derived from code length so the default unique-code alphabet is `0..MAX_CODE_LENGTH-1`.
constexpr uint8 PULSEEDITOR_MAX_DIGIT = PULSEEDITOR_MAX_CODE_LENGTH - 1;
/// Bucket count used for digit frequency arrays; rounded up so QPI arrays cover every supported digit.
constexpr uint8 PULSEEDITOR_DIGIT_BUCKETS = pulseEditorNextPowerOfTwo(PULSEEDITOR_MAX_DIGIT + 1);
/// Number of possible values on one payout-matrix axis: exact or misplaced matches from `0..MAX_CODE_LENGTH`.
constexpr uint8 PULSEEDITOR_MATRIX_SIDE = PULSEEDITOR_MAX_CODE_LENGTH + 1;
/// Logical payout matrix cell count before QPI power-of-two alignment.
constexpr uint16 PULSEEDITOR_PAYOUT_MATRIX_SIZE = (PULSEEDITOR_MAX_CODE_LENGTH + 1) * (PULSEEDITOR_MAX_CODE_LENGTH + 1);
/// Physical payout matrix capacity rounded up for QPI `Array` storage.
constexpr uint16 PULSEEDITOR_PAYOUT_MATRIX_CAPACITY = pulseEditorNextPowerOfTwo(PULSEEDITOR_PAYOUT_MATRIX_SIZE);
/// Platform fee percent deducted from each gross ticket purchase.
constexpr uint8 PULSEEDITOR_PLATFORM_FEE_PERCENT = 3;
/// Developer 1 share of the platform fee, expressed as percent of the platform fee.
constexpr uint8 PULSEEDITOR_PLATFORM_DEV1_SHARE_PERCENT = 25;
/// Developer 2 share of the platform fee, expressed as percent of the platform fee.
constexpr uint8 PULSEEDITOR_PLATFORM_DEV2_SHARE_PERCENT = 25;
/// Default upper bound for creator fee percent from non-platform ticket revenue.
constexpr uint8 PULSEEDITOR_DEFAULT_MAX_CREATOR_FEE_PERCENT = 20;
/// Hard upper bound for burn percent from non-platform ticket revenue.
constexpr uint8 PULSEEDITOR_MAX_BURN_PERCENT = 20;
/// Maximum retry count when generating unique random digits before deterministic fallback.
constexpr uint8 PULSEEDITOR_RANDOM_RETRY_LIMIT = 32;
/// Contract-share asset name used to distribute asset-entry dividends to PulseEditor shareholders.
constexpr uint64 PULSEEDITOR_CONTRACT_ASSET_NAME = 90500669654352ULL; // "PEDTOR"
/// Fixed-point scale for bonus multipliers; `12000` means `1.2x`.
constexpr uint32 PULSEEDITOR_BONUS_MULTIPLIER_SCALE = 10000;
/// Safety cap for configured bonus multipliers; `100000` means `10x`.
constexpr uint32 PULSEEDITOR_MAX_BONUS_MULTIPLIER_BPS = 100000;
/// Tick cadence for lifecycle automation; throttling avoids scanning template storage on every tick.
constexpr uint32 PULSEEDITOR_TICK_UPDATE_PERIOD = 100;
/// Bootstrap date sentinel used by QPI before calendar time is initialized.
constexpr uint32 PULSEEDITOR_DEFAULT_INIT_TIME = 22 << 9 | 4 << 5 | 13;
/// Maximum template slots inspected by the lifecycle automation during one throttled tick.
constexpr uint16 PULSEEDITOR_AUTOMATION_TEMPLATES_PER_TICK = 32;
/// Templates with no draws for this many epochs are deleted and their template-local funds are returned.
constexpr uint16 PULSEEDITOR_TEMPLATE_IDLE_EPOCH_LIMIT = 5;

struct PULSEEDITOR2
{
};

struct PULSEEDITOR : public ContractBase
{
public:
	enum class EReturnCode : uint8
	{
		SUCCESS,
		ACCESS_DENIED,
		INVALID_TEMPLATE,
		INVALID_STATE,
		INVALID_VALUE,
		INVALID_DIGITS,
		INSUFFICIENT_FUNDS,
		TICKET_INVALID_PRICE,
		TICKET_SOLD_OUT,
		PLAYER_TICKET_LIMIT,
		STORAGE_FULL,
		UNKNOWN_ERROR = UINT8_MAX
	};

	enum class ETemplateStatus : uint8
	{
		EMPTY,
		DRAFT,
		PUBLISHED,
		STOP_REQUESTED,
		STOPPED
	};

	enum class ERoundStatus : uint8
	{
		NONE,
		SELLING,
		CLOSED,
		SETTLED
	};

	enum class ETicketStatus : uint8
	{
		EMPTY,
		ACTIVE,
		PAID,
		UNPAID
	};

	/**
	 * @brief Reward currency used when paying base and bonus winnings.
	 */
	enum class ERewardMode : uint8
	{
		/// Winners are paid in Qubic from the template's Qubic reserves.
		QUBIC,
		/// Winners are paid in managed asset shares from the template's asset reserves.
		ASSET
	};

	/**
	 * @brief Currency source used when collecting ticket payments from players.
	 */
	enum class EEntryMode : uint8
	{
		/// Players pay ticket price with the Qubic invocation reward.
		QUBIC,
		/// Players pay ticket price with managed asset shares collected from their account.
		ASSET
	};

	/**
	 * @brief Converts a typed return code into the compact public ABI representation.
	 * @param code Internal enum value.
	 * @return `uint8` value returned by public procedures and functions.
	 */
	static constexpr uint8 toReturnCode(const EReturnCode& code) { return static_cast<uint8>(code); }

	/**
	 * Invariants:
	 * - Each template owns exactly one round slot; `currentRoundId` disambiguates tickets from previous rounds.
	 * - Ticket storage is append-only; global ticket indexes returned to clients remain stable.
	 * - Winner history is a ring buffer keyed by monotonic `winnerCounter`.
	 * - `prizeReserve` backs base fixed payouts, while `bonusReserve` backs multiplier bonus extras.
	 * - Asset-entry fee buckets are tracked per template because each template may use a different asset.
	 * - `lastDrawEpoch` stores the most recent draw epoch and acts as the idle-deletion baseline before the first draw.
	 * - Template mechanics and economy may not change while the active round is selling.
	 */
	struct GameTemplate
	{
		Array<uint64, PULSEEDITOR_PAYOUT_MATRIX_CAPACITY> payoutMatrix;
		Array<Asset, PULSEEDITOR_MAX_BONUS_ASSETS> bonusAssets;
		Array<uint8, 32> name;
		Asset rewardAsset;
		Asset entryAsset;
		id owner;
		uint64 ticketPrice;
		uint64 prizeReserve;
		uint64 bonusReserve;
		uint64 assetPrizeReserve;
		uint64 assetBonusReserve;
		uint64 assetEntryRevenue;
		uint64 assetCreatorRevenue;
		uint64 assetBurnAccrued;
		uint64 assetDeveloper1Accrued;
		uint64 assetDeveloper2Accrued;
		uint64 assetDividendAccrued;
		uint64 creatorRevenue;
		uint64 burnAccrued;
		uint64 totalRevenue;
		uint64 totalPaid;
		uint64 totalBonusPaid;
		uint64 totalAssetPaid;
		uint64 maxSinglePayout;
		uint32 currentRoundId;
		uint32 roundStartTick;
		uint32 roundEndTick;
		uint32 bonusMultiplierBps;
		uint16 ticketLimit;
		uint16 playerTicketLimit;
		uint16 bonusAssetCount;
		uint16 lastDrawEpoch;
		uint16 rewardOwnershipManagingContractIndex;
		uint16 rewardPossessionManagingContractIndex;
		uint16 bonusOwnershipManagingContractIndex;
		uint16 bonusPossessionManagingContractIndex;
		uint16 entryOwnershipManagingContractIndex;
		uint16 entryPossessionManagingContractIndex;
		uint8 codeLength;
		uint8 maxDigit;
		uint8 creatorFeePercent;
		uint8 burnPercent;
		ERewardMode rewardMode;
		EEntryMode entryMode;
		bit bonusEnabled;
		bit instantSettlement;
		bit allowRepeatedDigits;
		bit hasTicketSales;
		ETemplateStatus status;
	};

	struct Round
	{
		Array<uint8, PULSEEDITOR_DIGITS_ALIGNED> winningDigits;
		uint64 revenue;
		uint64 prizeAdded;
		uint64 paid;
		uint32 roundId;
		uint32 startTick;
		uint32 endTick;
		uint32 settledTick;
		uint16 ticketCount;
		uint16 winnerCount;
		uint16 unpaidWinnerCount;
		ERoundStatus status;
	};

	struct Ticket
	{
		Array<uint8, PULSEEDITOR_DIGITS_ALIGNED> digits;
		id player;
		uint64 payout;
		uint64 bonusPayout;
		uint64 assetPayout;
		uint64 assetBonusPayout;
		uint32 roundId;
		uint16 templateId;
		uint8 exact;
		uint8 misplaced;
		ETicketStatus status;
	};

	struct WinnerInfo
	{
		id player;
		uint64 payout;
		uint64 bonusPayout;
		uint64 assetPayout;
		uint64 assetBonusPayout;
		uint32 roundId;
		uint32 tick;
		uint16 templateId;
		uint16 epoch;
		uint8 exact;
		uint8 misplaced;
	};

	struct StateData
	{
		Array<GameTemplate, PULSEEDITOR_MAX_TEMPLATES> templates;
		Array<Round, PULSEEDITOR_MAX_TEMPLATES> rounds;
		Array<Ticket, PULSEEDITOR_MAX_TICKETS> tickets;
		Array<WinnerInfo, PULSEEDITOR_MAX_WINNERS> winners;
		id platformOwner;
		id developer1;
		id developer2;
		uint64 developer1Accrued;
		uint64 developer2Accrued;
		uint64 dividendAccrued;
		uint64 templateCount;
		uint64 ticketCount;
		uint64 winnerCounter;
		uint16 automationCursor;
		uint8 platformFeePercent;
		uint8 maxCreatorFeePercent;
	};

	/**
	 * @brief One ticket code used inside batched purchase input.
	 */
	struct TicketDigits
	{
		Array<uint8, PULSEEDITOR_DIGITS_ALIGNED> digits;
	};

	/**
	 * @brief Input for creating a draft game template.
	 * @param payoutMatrix Fixed payout table indexed by `(exact, misplaced)`.
	 * @param name Creator-provided display name stored as raw bytes.
	 * @param bonusAssets Optional assets; possession of any one qualifies a player for the multiplier bonus.
	 * @param rewardAsset Asset paid to winners when `rewardMode == ASSET`.
	 * @param entryAsset Asset collected from players when `entryMode == ASSET`.
	 * @param ticketPrice Exact ticket price in Qubic or entry-asset shares, depending on `entryMode`.
	 * @param bonusMultiplierBps Optional payout multiplier in basis points; `12000` means `1.2x`.
	 * @param roundStartTick Optional first tick at which ticket purchases are accepted; zero means immediate.
	 * @param roundEndTick Optional last selling tick; zero disables time-based auto-close.
	 * @param ticketLimit Maximum tickets accepted by each round of this template.
	 * @param playerTicketLimit Maximum tickets one player may buy in a round.
	 * @param bonusAssetCount Number of valid entries in `bonusAssets`.
	 * @param rewardOwnershipManagingContractIndex Ownership managing contract used for reward asset reserve/payout.
	 * @param rewardPossessionManagingContractIndex Possession managing contract used for reward asset reserve/payout.
	 * @param bonusOwnershipManagingContractIndex Ownership managing contract used for bonus asset checks.
	 * @param bonusPossessionManagingContractIndex Possession managing contract used for bonus asset checks.
	 * @param entryOwnershipManagingContractIndex Ownership managing contract used for asset ticket payments.
	 * @param entryPossessionManagingContractIndex Possession managing contract used for asset ticket payments.
	 * @param codeLength Number of digits players must submit.
	 * @param maxDigit Maximum allowed digit value; valid range is `0..maxDigit`.
	 * @param creatorFeePercent Creator share of non-platform ticket revenue.
	 * @param burnPercent Burn share of non-platform ticket revenue.
	 * @param rewardMode `QUBIC` for Qubic payouts or `ASSET` for asset-share payouts.
	 * @param entryMode `QUBIC` for invocation-reward ticket payments or `ASSET` for managed-share ticket payments.
	 * @param bonusEnabled Whether multiplier bonus checks and reserve accounting are enabled.
	 * @param instantSettlement Whether each accepted ticket closes and settles its own round immediately.
	 * @param allowRepeatedDigits Whether the same digit may appear more than once in a code.
	 */
	struct CreateTemplate_input
	{
		Array<uint64, PULSEEDITOR_PAYOUT_MATRIX_CAPACITY> payoutMatrix;
		Array<Asset, PULSEEDITOR_MAX_BONUS_ASSETS> bonusAssets;
		Array<uint8, 32> name;
		Asset rewardAsset;
		Asset entryAsset;
		uint64 ticketPrice;
		uint32 roundStartTick;
		uint32 roundEndTick;
		uint32 bonusMultiplierBps;
		uint16 ticketLimit;
		uint16 playerTicketLimit;
		uint16 bonusAssetCount;
		uint16 rewardOwnershipManagingContractIndex;
		uint16 rewardPossessionManagingContractIndex;
		uint16 bonusOwnershipManagingContractIndex;
		uint16 bonusPossessionManagingContractIndex;
		uint16 entryOwnershipManagingContractIndex;
		uint16 entryPossessionManagingContractIndex;
		uint8 codeLength;
		uint8 maxDigit;
		uint8 creatorFeePercent;
		uint8 burnPercent;
		ERewardMode rewardMode;
		EEntryMode entryMode;
		bit bonusEnabled;
		bit instantSettlement;
		bit allowRepeatedDigits;
	};

	/**
	 * @brief Output from template creation.
	 * @param requiredPrizeReserve Minimum base reserve required before publication.
	 * @param requiredBonusReserve Minimum multiplier bonus reserve required before publication.
	 * @param templateId New template index when creation succeeds.
	 * @param returnCode `SUCCESS` or the validation/storage error.
	 */
	struct CreateTemplate_output
	{
		uint64 requiredPrizeReserve;
		uint64 requiredBonusReserve;
		uint16 templateId;
		uint8 returnCode;
	};

	/**
	 * @brief Input for editing an unpublished draft game template.
	 * @param payoutMatrix Replacement fixed payout table indexed by `(exact, misplaced)`.
	 * @param name Replacement display name stored as raw bytes.
	 * @param bonusAssets Replacement assets; possession of any one qualifies a player for the multiplier bonus.
	 * @param rewardAsset Replacement asset paid to winners when `rewardMode == ASSET`.
	 * @param entryAsset Replacement asset collected from players when `entryMode == ASSET`.
	 * @param ticketPrice Replacement ticket price in Qubic or entry-asset shares, depending on `entryMode`.
	 * @param bonusMultiplierBps Replacement payout multiplier in basis points.
	 * @param roundStartTick Replacement first selling tick; zero means immediate.
	 * @param roundEndTick Replacement last selling tick; zero disables time-based auto-close.
	 * @param templateId Draft template owned by the invocator.
	 * @param ticketLimit Replacement maximum tickets accepted by each round.
	 * @param playerTicketLimit Replacement maximum tickets one player may buy in a round.
	 * @param bonusAssetCount Replacement number of valid entries in `bonusAssets`.
	 * @param rewardOwnershipManagingContractIndex Replacement ownership managing contract for reward asset reserve/payout.
	 * @param rewardPossessionManagingContractIndex Replacement possession managing contract for reward asset reserve/payout.
	 * @param bonusOwnershipManagingContractIndex Replacement ownership managing contract for bonus checks.
	 * @param bonusPossessionManagingContractIndex Replacement possession managing contract for bonus checks.
	 * @param entryOwnershipManagingContractIndex Replacement ownership managing contract for asset ticket payments.
	 * @param entryPossessionManagingContractIndex Replacement possession managing contract for asset ticket payments.
	 * @param codeLength Replacement number of digits players must submit.
	 * @param maxDigit Replacement maximum allowed digit value.
	 * @param creatorFeePercent Replacement creator share of non-platform ticket revenue.
	 * @param burnPercent Replacement burn share of non-platform ticket revenue.
	 * @param rewardMode Replacement reward payout mode.
	 * @param entryMode Replacement ticket payment mode.
	 * @param bonusEnabled Replacement multiplier-bonus enable flag.
	 * @param instantSettlement Replacement instant-settlement flag.
	 * @param allowRepeatedDigits Replacement duplicate-digit policy.
	 * @note Selling rounds are immutable; published templates may only be changed after the active round settles.
	 */
	struct UpdateTemplate_input
	{
		Array<uint64, PULSEEDITOR_PAYOUT_MATRIX_CAPACITY> payoutMatrix;
		Array<Asset, PULSEEDITOR_MAX_BONUS_ASSETS> bonusAssets;
		Array<uint8, 32> name;
		Asset rewardAsset;
		Asset entryAsset;
		uint64 ticketPrice;
		uint32 roundStartTick;
		uint32 roundEndTick;
		uint32 bonusMultiplierBps;
		uint16 templateId;
		uint16 ticketLimit;
		uint16 playerTicketLimit;
		uint16 bonusAssetCount;
		uint16 rewardOwnershipManagingContractIndex;
		uint16 rewardPossessionManagingContractIndex;
		uint16 bonusOwnershipManagingContractIndex;
		uint16 bonusPossessionManagingContractIndex;
		uint16 entryOwnershipManagingContractIndex;
		uint16 entryPossessionManagingContractIndex;
		uint8 codeLength;
		uint8 maxDigit;
		uint8 creatorFeePercent;
		uint8 burnPercent;
		ERewardMode rewardMode;
		EEntryMode entryMode;
		bit bonusEnabled;
		bit instantSettlement;
		bit allowRepeatedDigits;
	};

	/**
	 * @brief Output from draft template update.
	 * @param requiredPrizeReserve Recomputed minimum base reserve required before publication.
	 * @param requiredBonusReserve Recomputed minimum multiplier bonus reserve required before publication.
	 * @param returnCode `SUCCESS`, `INVALID_TEMPLATE`, `ACCESS_DENIED`, `INVALID_STATE`, or `INVALID_VALUE`.
	 */
	struct UpdateTemplate_output
	{
		uint64 requiredPrizeReserve;
		uint64 requiredBonusReserve;
		uint8 returnCode;
	};

	/**
	 * @brief Input for adding Qubic to a template's prize reserve.
	 * @param templateId Target template owned by the invocator.
	 * @note The deposited amount is the invocation reward attached to the call.
	 */
	struct DepositPrizeReserve_input
	{
		uint16 templateId;
	};

	/**
	 * @brief Output from prize reserve deposit.
	 * @param depositedAmount Amount accepted from the invocation reward.
	 * @param prizeReserve Updated reserve after the deposit.
	 * @param returnCode `SUCCESS` or the rejection reason.
	 */
	struct DepositPrizeReserve_output
	{
		uint64 depositedAmount;
		uint64 prizeReserve;
		uint8 returnCode;
	};

	/**
	 * @brief Input for adding Qubic to a template's multiplier-bonus reserve.
	 * @param templateId Target template owned by the invocator.
	 * @note The deposited amount is the invocation reward attached to the call.
	 */
	struct DepositBonusReserve_input
	{
		uint16 templateId;
	};

	/**
	 * @brief Output from bonus reserve deposit.
	 * @param depositedAmount Amount accepted from the invocation reward.
	 * @param bonusReserve Updated multiplier-bonus reserve after the deposit.
	 * @param returnCode `SUCCESS` or the rejection reason.
	 */
	struct DepositBonusReserve_output
	{
		uint64 depositedAmount;
		uint64 bonusReserve;
		uint8 returnCode;
	};

	/**
	 * @brief Input for transferring reward asset shares into a template reserve.
	 * @param templateId Target template owned by the invocator.
	 * @param numberOfShares Asset shares to transfer from the invocator to this contract.
	 * @param depositToBonusReserve Whether shares fund multiplier bonuses instead of base rewards.
	 * @note The template must use `ASSET` reward mode and this contract must manage the transferred asset.
	 */
	struct DepositAssetReserve_input
	{
		uint64 numberOfShares;
		uint16 templateId;
		bit depositToBonusReserve;
	};

	/**
	 * @brief Output from reward asset reserve deposit.
	 * @param depositedNumberOfShares Asset shares accepted by the contract.
	 * @param assetPrizeReserve Updated base asset reward reserve.
	 * @param assetBonusReserve Updated multiplier-bonus asset reserve.
	 * @param returnCode `SUCCESS` or the rejection reason.
	 */
	struct DepositAssetReserve_output
	{
		uint64 depositedNumberOfShares;
		uint64 assetPrizeReserve;
		uint64 assetBonusReserve;
		uint8 returnCode;
	};

	/**
	 * @brief Input for publishing a funded draft template.
	 * @param templateId Draft template to publish.
	 */
	struct PublishTemplate_input
	{
		uint16 templateId;
	};

	/**
	 * @brief Output from template publication.
	 * @param roundId First selling round id created by the publication.
	 * @param returnCode `SUCCESS` or the publication error.
	 */
	struct PublishTemplate_output
	{
		uint32 roundId;
		uint8 returnCode;
	};

	/**
	 * @brief Input for buying one ticket in the current selling round.
	 * @param digits Player-submitted code; only the first template `codeLength` entries are used.
	 * @param templateId Published template whose active round receives the ticket.
	 * @note Qubic-entry templates pay Qubic rewards; asset-entry templates collect and pay the same managed asset.
	 */
	struct BuyTicket_input
	{
		Array<uint8, PULSEEDITOR_DIGITS_ALIGNED> digits;
		uint16 templateId;
	};

	/**
	 * @brief Output from ticket purchase.
	 * @param roundId Active round id that accepted the ticket.
	 * @param ticketIndex Global ticket array index assigned to the purchase.
	 * @param returnCode `SUCCESS` or the purchase rejection reason.
	 */
	struct BuyTicket_output
	{
		uint64 ticketIndex;
		uint32 roundId;
		uint8 returnCode;
	};

	/**
	 * @brief Input for buying several tickets in one call.
	 * @param tickets Submitted codes; only the first `ticketCount` entries are used.
	 * @param templateId Published template whose active round receives the tickets.
	 * @param ticketCount Number of tickets to buy, capped by `PULSEEDITOR_MAX_BATCH_TICKETS`.
	 * @note Qubic-entry batches pay Qubic rewards; asset-entry batches collect and pay the same managed asset.
	 * @warning Instant batches settle tickets sequentially, so a later failure can leave earlier tickets accepted and paid.
	 */
	struct BuyTickets_input
	{
		Array<TicketDigits, PULSEEDITOR_MAX_BATCH_TICKETS> tickets;
		uint16 templateId;
		uint16 ticketCount;
	};

	/**
	 * @brief Output from batched ticket purchase.
	 * @param ticketIndexes Global ticket array indexes assigned to accepted tickets.
	 * @param roundId First round id that accepted a ticket.
	 * @param acceptedCount Number of tickets stored; non-instant failures keep this at zero, instant failures may return a partial count.
	 * @param returnCode `SUCCESS` or the purchase rejection reason.
	 */
	struct BuyTickets_output
	{
		Array<uint64, PULSEEDITOR_MAX_BATCH_TICKETS> ticketIndexes;
		uint32 roundId;
		uint16 acceptedCount;
		uint8 returnCode;
	};

	/**
	 * @brief Internal input for settling the current selling round from tick automation.
	 * @param templateId Template whose active round should be settled.
	 */
	struct SettleRound_input
	{
		uint16 templateId;
	};

	/**
	 * @brief Internal output from settlement.
	 * @param winningDigits Generated winning code stored for the settled round.
	 * @param totalPaid Total Qubic paid to winners during this settlement.
	 * @param winnerCount Number of tickets with a positive fixed payout.
	 * @param unpaidWinnerCount Winners that could not be paid because the prize reserve was insufficient.
	 * @param returnCode `SUCCESS` or the settlement rejection reason.
	 */
	struct SettleRound_output
	{
		Array<uint8, PULSEEDITOR_DIGITS_ALIGNED> winningDigits;
		uint64 totalPaid;
		uint16 winnerCount;
		uint16 unpaidWinnerCount;
		uint8 returnCode;
	};

	/**
	 * @brief Input for requesting a graceful template stop.
	 * @param templateId Template owned by the invocator.
	 */
	struct RequestStop_input
	{
		uint16 templateId;
	};

	/**
	 * @brief Output from stop request.
	 * @param returnCode `SUCCESS` or the access/state rejection reason.
	 */
	struct RequestStop_output
	{
		uint8 returnCode;
	};

	/**
	 * @brief Input for withdrawing creator revenue accumulated by ticket purchases.
	 * @param templateId Template whose creator balance is used.
	 * @param amount Qubic amount or asset-share amount to transfer to the template owner.
	 */
	struct WithdrawCreatorRevenue_input
	{
		uint16 templateId;
		uint64 amount;
	};

	/**
	 * @brief Output from creator revenue withdrawal.
	 * @param withdrawnAmount Amount transferred to the invocator.
	 * @param remainingRevenue Creator revenue left after withdrawal.
	 * @param returnCode `SUCCESS` or the withdrawal rejection reason.
	 */
	struct WithdrawCreatorRevenue_output
	{
		uint64 withdrawnAmount;
		uint64 remainingRevenue;
		uint8 returnCode;
	};

	/**
	 * @brief Input for withdrawing asset-denominated platform revenue from one template.
	 * @param templateId Template whose asset platform balances are used.
	 * @note Asset dividends are paid to holders of the PulseEditor contract-share asset.
	 */
	struct WithdrawAssetPlatformRevenue_input
	{
		uint16 templateId;
	};

	/**
	 * @brief Output from asset platform revenue withdrawal.
	 * @param developer1Amount Asset shares transferred to developer 1.
	 * @param developer2Amount Asset shares transferred to developer 2.
	 * @param dividendAmount Asset shares distributed to PulseEditor shareholders.
	 * @param dividendAccrued Asset shares retained for future dividend handling.
	 * @param returnCode `SUCCESS` or the withdrawal rejection reason.
	 */
	struct WithdrawAssetPlatformRevenue_output
	{
		uint64 developer1Amount;
		uint64 developer2Amount;
		uint64 dividendAmount;
		uint64 dividendAccrued;
		uint8 returnCode;
	};

	/**
	 * @brief Input for initializing or updating platform fee recipients and creator limits.
	 * @param platformOwner Owner allowed to update platform config and withdraw platform revenue.
	 * @param developer1 First developer fee recipient.
	 * @param developer2 Second developer fee recipient.
	 * @param maxCreatorFeePercent Maximum creator fee allowed for future templates.
	 */
	struct SetPlatformConfig_input
	{
		id platformOwner;
		id developer1;
		id developer2;
		uint8 maxCreatorFeePercent;
	};

	/**
	 * @brief Output from platform config update.
	 * @param returnCode `SUCCESS` or the access/value rejection reason.
	 */
	struct SetPlatformConfig_output
	{
		uint8 returnCode;
	};

	/**
	 * @brief Empty input for withdrawing accumulated platform revenue.
	 * @note Only the configured platform owner may call this procedure successfully.
	 */
	struct WithdrawPlatformRevenue_input
	{
	};

	/**
	 * @brief Output from platform revenue withdrawal.
	 * @param developer1Amount Amount transferred to developer 1.
	 * @param developer2Amount Amount transferred to developer 2.
	 * @param dividendAmount Total amount distributed to contract shareholders.
	 * @param returnCode `SUCCESS` or the withdrawal rejection reason.
	 */
	struct WithdrawPlatformRevenue_output
	{
		uint64 developer1Amount;
		uint64 developer2Amount;
		uint64 dividendAmount;
		uint8 returnCode;
	};

	/**
	 * @brief Internal input for opening the next round after settlement.
	 * @param templateId Published template whose next round should open.
	 */
	struct StartNextRound_input
	{
		uint16 templateId;
	};

	/**
	 * @brief Internal output from next-round creation.
	 * @param roundId New active round id.
	 * @param returnCode `SUCCESS` or the state/funding rejection reason.
	 */
	struct StartNextRound_output
	{
		uint32 roundId;
		uint8 returnCode;
	};

	/**
	 * @brief Input for querying a template.
	 * @param templateId Template index to read.
	 */
	struct GetTemplate_input
	{
		uint16 templateId;
	};

	/**
	 * @brief Output from template query.
	 * @param gameTemplate Full stored template snapshot.
	 * @param returnCode `SUCCESS` or `INVALID_TEMPLATE`.
	 */
	struct GetTemplate_output
	{
		GameTemplate gameTemplate;
		uint8 returnCode;
	};

	/**
	 * @brief Input for querying the current round of a template.
	 * @param templateId Template whose round slot should be read.
	 */
	struct GetRound_input
	{
		uint16 templateId;
	};

	/**
	 * @brief Output from current-round query.
	 * @param round Current round snapshot for the template.
	 * @param returnCode `SUCCESS` or `INVALID_TEMPLATE`.
	 */
	struct GetRound_output
	{
		Round round;
		uint8 returnCode;
	};

	/**
	 * @brief Input for checking whether a template can be published or started.
	 * @param templateId Template index to inspect.
	 */
	struct GetTemplateReadiness_input
	{
		uint16 templateId;
	};

	/**
	 * @brief Output from template readiness query.
	 * @param requiredPrizeReserve Minimum base prize reserve required for one funded round.
	 * @param requiredBonusReserve Minimum bonus reserve required when multiplier bonuses are enabled.
	 * @param prizeReserve Current base prize reserve.
	 * @param bonusReserve Current multiplier-bonus reserve.
	 * @param assetPrizeReserve Current base asset reward reserve.
	 * @param assetBonusReserve Current multiplier-bonus asset reserve.
	 * @param currentTick Tick observed while evaluating time-window readiness.
	 * @param reasonCode `SUCCESS` when ready, otherwise the first blocker.
	 * @param returnCode `SUCCESS` or `INVALID_TEMPLATE`.
	 * @param isReady True when current funding and lifecycle state allow publication or next-round start.
	 */
	struct GetTemplateReadiness_output
	{
		uint64 requiredPrizeReserve;
		uint64 requiredBonusReserve;
		uint64 prizeReserve;
		uint64 bonusReserve;
		uint64 assetPrizeReserve;
		uint64 assetBonusReserve;
		uint32 currentTick;
		uint8 reasonCode;
		uint8 returnCode;
		bit isReady;
	};

	/**
	 * @brief Input for reading a page of created template ids.
	 * @param offset Zero-based template offset.
	 * @param limit Requested number of ids; zero means `PULSEEDITOR_TEMPLATES_PAGE_SIZE`.
	 */
	struct GetTemplates_input
	{
		uint64 offset;
		uint16 limit;
	};

	/**
	 * @brief Output from template discovery query.
	 * @param templateIds Page of template ids.
	 * @param statuses Status byte for each returned template.
	 * @param totalTemplates Number of templates ever created.
	 * @param returnedCount Number of valid entries in `templateIds` and `statuses`.
	 * @param returnCode `SUCCESS` or `INVALID_VALUE`.
	 */
	struct GetTemplates_output
	{
		Array<uint16, PULSEEDITOR_TEMPLATES_PAGE_SIZE> templateIds;
		Array<uint8, PULSEEDITOR_TEMPLATES_PAGE_SIZE> statuses;
		uint64 totalTemplates;
		uint16 returnedCount;
		uint8 returnCode;
	};

	/**
	 * @brief Input for querying a stored ticket by global index.
	 * @param ticketIndex Index previously returned by `BuyTicket`.
	 */
	struct GetTicket_input
	{
		uint64 ticketIndex;
	};

	/**
	 * @brief Output from ticket query.
	 * @param ticket Stored ticket snapshot.
	 * @param returnCode `SUCCESS` or `INVALID_VALUE`.
	 */
	struct GetTicket_output
	{
		Ticket ticket;
		uint8 returnCode;
	};

	/**
	 * @brief Input for querying a chronological page of tickets bought by one player.
	 * @param player Player id whose tickets should be scanned.
	 * @param offset Zero-based offset within the matched ticket set.
	 * @param roundId Optional round id filter, used only when `useRoundFilter` is true.
	 * @param templateId Optional template filter, used only when `useTemplateFilter` is true.
	 * @param limit Requested number of entries; zero means `PULSEEDITOR_TICKETS_PAGE_SIZE`.
	 * @param useTemplateFilter Whether to restrict results to `templateId`.
	 * @param useRoundFilter Whether to restrict results to `roundId`.
	 * @note Tickets are returned in global insertion order, which is chronological for accepted purchases.
	 */
	struct GetPlayerTickets_input
	{
		id player;
		uint64 offset;
		uint32 roundId;
		uint16 templateId;
		uint16 limit;
		bit useTemplateFilter;
		bit useRoundFilter;
	};

	/**
	 * @brief Output from paged player-ticket query.
	 * @param tickets Page of matched ticket snapshots.
	 * @param ticketIndexes Global indexes corresponding to the returned tickets.
	 * @param totalMatched Total tickets matching the player and optional filters.
	 * @param returnedCount Number of valid entries in `tickets` and `ticketIndexes`.
	 * @param returnCode `SUCCESS`, `INVALID_TEMPLATE`, or `INVALID_VALUE`.
	 */
	struct GetPlayerTickets_output
	{
		Array<Ticket, PULSEEDITOR_TICKETS_PAGE_SIZE> tickets;
		Array<uint64, PULSEEDITOR_TICKETS_PAGE_SIZE> ticketIndexes;
		uint64 totalMatched;
		uint16 returnedCount;
		uint8 returnCode;
	};

	/**
	 * @brief Input for querying a page from the winner history ring buffer.
	 * @param offset Zero-based offset from the oldest retained winner.
	 * @param limit Requested number of entries; zero means `PULSEEDITOR_WINNERS_PAGE_SIZE`.
	 * @note Returned entries are ordered from oldest to newest within the retained history window.
	 */
	struct GetWinners_input
	{
		uint64 offset;
		uint16 limit;
	};

	/**
	 * @brief Output from paged winner-history query.
	 * @param winners Page of winner entries starting at `input.offset`.
	 * @param winnerCounter Monotonic winner counter used to interpret ring-buffer order.
	 * @param totalStored Number of winner entries currently retained by the ring buffer.
	 * @param returnedCount Number of valid entries in `winners`.
	 * @param returnCode `SUCCESS` or `INVALID_VALUE` when the offset is outside the retained window.
	 */
	struct GetWinners_output
	{
		Array<WinnerInfo, PULSEEDITOR_WINNERS_PAGE_SIZE> winners;
		uint64 winnerCounter;
		uint64 totalStored;
		uint16 returnedCount;
		uint8 returnCode;
	};

	/**
	 * @brief Empty input for querying platform fee accounting.
	 */
	struct GetPlatformAccounting_input
	{
	};

	/**
	 * @brief Output from platform accounting query.
	 * @param platformOwner Current platform owner.
	 * @param developer1 Current first developer recipient.
	 * @param developer2 Current second developer recipient.
	 * @param developer1Accrued Pending amount owed to developer 1.
	 * @param developer2Accrued Pending amount owed to developer 2.
	 * @param dividendAccrued Pending total amount reserved for shareholder dividends.
	 * @param platformFeePercent Platform fee percent deducted from tickets.
	 * @param maxCreatorFeePercent Current creator-fee upper bound.
	 * @param returnCode Always `SUCCESS` for the MVP getter.
	 */
	struct GetPlatformAccounting_output
	{
		id platformOwner;
		id developer1;
		id developer2;
		uint64 developer1Accrued;
		uint64 developer2Accrued;
		uint64 dividendAccrued;
		uint8 platformFeePercent;
		uint8 maxCreatorFeePercent;
		uint8 returnCode;
	};

	/**
	 * @brief Input for validating a submitted or generated code.
	 * @param digits Code digits; only the first `codeLength` entries are checked.
	 * @param codeLength Active code length.
	 * @param maxDigit Maximum allowed digit value.
	 * @param allowRepeatedDigits Whether duplicate digits are accepted.
	 */
	struct ValidateDigits_input
	{
		Array<uint8, PULSEEDITOR_DIGITS_ALIGNED> digits;
		uint8 codeLength;
		uint8 maxDigit;
		bit allowRepeatedDigits;
	};

	/**
	 * @brief Output from digit validation.
	 * @param isValid True when the code obeys range and uniqueness rules.
	 * @param returnCode `SUCCESS` or `INVALID_DIGITS`.
	 */
	struct ValidateDigits_output
	{
		bit isValid;
		uint8 returnCode;
	};

	/**
	 * @brief Input for releasing asset share management rights from this contract.
	 * @param asset Asset whose management rights should be moved.
	 * @param numberOfShares Number of shares to release.
	 * @param newManagingContractIndex Destination ownership and possession managing contract index.
	 */
	struct TransferShareManagementRights_input
	{
		Asset asset;
		sint64 numberOfShares;
		uint32 newManagingContractIndex;
	};

	/**
	 * @brief Output from share-management-rights release.
	 * @param transferredNumberOfShares Number of shares released, or zero on failure.
	 */
	struct TransferShareManagementRights_output
	{
		sint64 transferredNumberOfShares;
	};

	struct TransferShareManagementRights_locals
	{
		sint64 result;
		sint64 reward;
		sint64 refundAmount;
		bit success;
	};

	/**
	 * @brief Input for internal exact/misplaced match counting.
	 * @param playerDigits Player code.
	 * @param winningDigits Generated winning code.
	 * @param codeLength Number of positions to compare.
	 */
	struct CountMatches_input
	{
		Array<uint8, PULSEEDITOR_DIGITS_ALIGNED> playerDigits;
		Array<uint8, PULSEEDITOR_DIGITS_ALIGNED> winningDigits;
		uint8 codeLength;
	};

	/**
	 * @brief Output from internal match counting.
	 * @param payoutMatrixIndex Linear payout matrix index for `(exact, misplaced)`.
	 * @param exact Number of position-correct digits.
	 * @param misplaced Number of value-correct but position-wrong digits.
	 */
	struct CountMatches_output
	{
		uint16 payoutMatrixIndex;
		uint8 exact;
		uint8 misplaced;
	};

	/**
	 * @brief Input for deterministic winning-code generation.
	 * @param seed K12-derived round seed.
	 * @param codeLength Number of digits to generate.
	 * @param maxDigit Maximum generated digit value.
	 * @param allowRepeatedDigits Whether generated digits may repeat.
	 */
	struct GenerateWinningDigits_input
	{
		uint64 seed;
		uint8 codeLength;
		uint8 maxDigit;
		bit allowRepeatedDigits;
	};

	/**
	 * @brief Output from deterministic winning-code generation.
	 * @param digits Generated winning digits in QPI-aligned storage.
	 */
	struct GenerateWinningDigits_output
	{
		Array<uint8, PULSEEDITOR_DIGITS_ALIGNED> digits;
	};

	struct SettleInstantTicket_input
	{
		uint64 ticketIndex;
		uint16 templateId;
	};

	struct SettleInstantTicket_output
	{
		uint8 returnCode;
	};

	struct TransferAssetDividendToShareholders_input
	{
		Asset dividendAsset;
		Asset shareholdersAsset;
		sint64 dividendAmount;
		sint64 shareholdersTotalShares;
	};

	struct TransferAssetDividendToShareholders_output
	{
		uint64 distributedAmount;
	};

	/**
	 * @brief Internal input for deleting an idle template.
	 * @param templateId Template whose local balances should be refunded before clearing storage.
	 */
	struct DeleteIdleTemplate_input
	{
		uint16 templateId;
	};

	/**
	 * @brief Internal output from idle-template deletion.
	 * @param qubicRefund Qubic amount returned to the template owner.
	 * @param assetRefund Asset shares returned to the template owner.
	 * @param returnCode `SUCCESS` or the deletion blocker.
	 */
	struct DeleteIdleTemplate_output
	{
		uint64 qubicRefund;
		uint64 assetRefund;
		uint8 returnCode;
	};

	/**
	 * @brief Empty input for the throttled lifecycle automation pass.
	 */
	struct ProcessLifecycleAutomation_input
	{
	};

	/**
	 * @brief Output counters from one lifecycle automation pass.
	 * @param inspectedTemplates Number of template slots checked in this pass.
	 * @param lifecycleActions Number of settle/start actions attempted in this pass.
	 */
	struct ProcessLifecycleAutomation_output
	{
		uint16 inspectedTemplates;
		uint16 lifecycleActions;
	};

	struct ValidateDigits_locals
	{
		Array<uint8, PULSEEDITOR_DIGIT_BUCKETS> seen;
		uint64 i;
		uint8 digit;
		uint8 count;
	};

	struct CountMatches_locals
	{
		Array<uint8, PULSEEDITOR_DIGIT_BUCKETS> playerCounts;
		Array<uint8, PULSEEDITOR_DIGIT_BUCKETS> winningCounts;
		uint64 i;
		uint8 playerDigit;
		uint8 winningDigit;
		uint8 playerCount;
		uint8 winningCount;
	};

	struct GenerateWinningDigits_locals
	{
		Array<uint8, PULSEEDITOR_DIGIT_BUCKETS> used;
		uint64 index;
		uint64 tempValue;
		uint8 candidate;
		uint8 attempts;
		uint8 fallback;
	};

	struct CreateTemplate_locals
	{
		GameTemplate gameTemplate;
		uint64 i;
		uint16 templateId;
		uint64 payout;
	};

	struct UpdateTemplate_locals
	{
		GameTemplate gameTemplate;
		Round round;
		uint64 i;
		uint64 payout;
	};

	struct DepositPrizeReserve_locals
	{
		GameTemplate gameTemplate;
		uint64 depositAmount;
	};

	struct DepositBonusReserve_locals
	{
		GameTemplate gameTemplate;
		uint64 depositAmount;
	};

	struct DepositAssetReserve_locals
	{
		GameTemplate gameTemplate;
		sint64 transferResult;
		sint64 possessedShares;
	};

	struct PublishTemplate_locals
	{
		GameTemplate gameTemplate;
		Round round;
	};

	struct BuyTicket_locals
	{
		GameTemplate gameTemplate;
		Round round;
		Ticket ticket;
		SettleInstantTicket_input settleInput;
		SettleInstantTicket_output settleOutput;
		ValidateDigits_input validateInput;
		ValidateDigits_output validateOutput;
		uint64 i;
		uint64 platformFee;
		uint64 dev1Amount;
		uint64 dev2Amount;
		uint64 dividendAmount;
		uint64 netRevenue;
		uint64 creatorAmount;
		uint64 burnAmount;
		uint64 prizeAmount;
		uint64 reward;
		sint64 transferResult;
		sint64 possessedShares;
		uint16 playerTicketCount;
	};

	struct BuyTickets_locals
	{
		GameTemplate gameTemplate;
		Round round;
		Ticket ticket;
		SettleInstantTicket_input settleInput;
		SettleInstantTicket_output settleOutput;
		ValidateDigits_input validateInput;
		ValidateDigits_output validateOutput;
		uint64 i;
		uint64 j;
		uint64 platformFee;
		uint64 dev1Amount;
		uint64 dev2Amount;
		uint64 dividendAmount;
		uint64 netRevenue;
		uint64 creatorAmount;
		uint64 burnAmount;
		uint64 prizeAmount;
		uint64 totalPrice;
		uint64 reward;
		sint64 transferResult;
		sint64 possessedShares;
		uint16 playerTicketCount;
	};

	struct SettleRound_randomData
	{
		m256i prevSpectrumDigest;
		uint32 roundId;
		uint16 templateId;
		uint16 ticketCount;
	};

	struct SettleRound_locals
	{
		SettleRound_randomData randomData;
		GameTemplate gameTemplate;
		Round round;
		Ticket ticket;
		WinnerInfo winnerInfo;
		GenerateWinningDigits_input generateInput;
		GenerateWinningDigits_output generateOutput;
		CountMatches_input countInput;
		CountMatches_output countOutput;
		uint64 i;
		uint64 winnerIndex;
		uint64 payout;
		uint64 bonusPayout;
		uint64 totalPayout;
		uint64 seed;
		sint64 bonusShares;
		sint64 transferResult;
		uint16 bonusAssetIndex;
		bit bonusQualified;
	};

	struct SettleInstantTicket_locals
	{
		SettleRound_randomData randomData;
		GameTemplate gameTemplate;
		Round round;
		Ticket ticket;
		WinnerInfo winnerInfo;
		GenerateWinningDigits_input generateInput;
		GenerateWinningDigits_output generateOutput;
		CountMatches_input countInput;
		CountMatches_output countOutput;
		uint64 winnerIndex;
		uint64 payout;
		uint64 bonusPayout;
		uint64 totalPayout;
		uint64 seed;
		sint64 bonusShares;
		sint64 transferResult;
		uint16 bonusAssetIndex;
		bit bonusQualified;
	};

	struct TransferAssetDividendToShareholders_locals
	{
		AssetPossessionIterator shareholdersIter;
		sint64 dividendPerShare;
		sint64 holderShares;
		sint64 holderDividend;
		sint64 transferResult;
	};

	struct RequestStop_locals
	{
		GameTemplate gameTemplate;
		Round round;
	};

	struct WithdrawCreatorRevenue_locals
	{
		GameTemplate gameTemplate;
		uint64 amount;
		sint64 transferResult;
	};

	struct WithdrawAssetPlatformRevenue_locals
	{
		GameTemplate gameTemplate;
		TransferAssetDividendToShareholders_input transferInput;
		TransferAssetDividendToShareholders_output transferOutput;
		uint64 developer1Amount;
		uint64 developer2Amount;
		uint64 dividendAmount;
		uint64 totalAmount;
		sint64 transferResult;
		sint64 possessedShares;
	};

	struct WithdrawPlatformRevenue_locals
	{
		uint64 developer1Amount;
		uint64 developer2Amount;
		uint64 dividendAmount;
		uint64 dividendPerShare;
	};

	struct StartNextRound_locals
	{
		GameTemplate gameTemplate;
		Round round;
	};

	struct ProcessLifecycleAutomation_locals
	{
		GameTemplate gameTemplate;
		Round round;
		SettleRound_input settleInput;
		SettleRound_output settleOutput;
		StartNextRound_input startInput;
		StartNextRound_output startOutput;
		DeleteIdleTemplate_input deleteInput;
		DeleteIdleTemplate_output deleteOutput;
		uint64 i;
		uint64 templateIndex;
		uint64 templatesToInspect;
	};

	struct DeleteIdleTemplate_locals
	{
		GameTemplate gameTemplate;
		Round round;
		uint64 qubicRefund;
		uint64 assetRefund;
		sint64 transferResult;
	};

	struct BEGIN_TICK_locals
	{
		ProcessLifecycleAutomation_input automationInput;
		ProcessLifecycleAutomation_output automationOutput;
		uint32 currentDateStamp;
	};

	struct GetWinners_locals
	{
		uint64 oldestCounter;
		uint64 sourceCounter;
		uint64 sourceIndex;
		uint64 remaining;
		uint16 requestedLimit;
		uint16 i;
	};

	struct GetPlayerTickets_locals
	{
		Ticket ticket;
		uint64 i;
		uint64 matchedCount;
		uint16 requestedLimit;
	};

	struct GetTemplateReadiness_locals
	{
		GameTemplate gameTemplate;
		Round round;
	};

	struct GetTemplates_locals
	{
		uint64 remaining;
		uint16 requestedLimit;
		uint16 i;
	};

	/**
	 * @brief Registers PulseEditor public ABI procedures and functions.
	 * @note Procedure/function indices are part of the contract ABI and must stay stable once published.
	 */
	REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
	{
		REGISTER_USER_PROCEDURE(CreateTemplate, 1);
		REGISTER_USER_PROCEDURE(DepositPrizeReserve, 2);
		REGISTER_USER_PROCEDURE(PublishTemplate, 3);
		REGISTER_USER_PROCEDURE(BuyTicket, 4);
		REGISTER_USER_PROCEDURE(RequestStop, 6);
		REGISTER_USER_PROCEDURE(WithdrawCreatorRevenue, 7);
		REGISTER_USER_PROCEDURE(SetPlatformConfig, 8);
		REGISTER_USER_PROCEDURE(WithdrawPlatformRevenue, 9);
		REGISTER_USER_PROCEDURE(TransferShareManagementRights, 11);
		REGISTER_USER_PROCEDURE(UpdateTemplate, 12);
		REGISTER_USER_PROCEDURE(DepositBonusReserve, 13);
		REGISTER_USER_PROCEDURE(DepositAssetReserve, 14);
		REGISTER_USER_PROCEDURE(BuyTickets, 15);
		REGISTER_USER_PROCEDURE(WithdrawAssetPlatformRevenue, 16);

		REGISTER_USER_FUNCTION(GetTemplate, 1);
		REGISTER_USER_FUNCTION(GetRound, 2);
		REGISTER_USER_FUNCTION(GetTicket, 3);
		REGISTER_USER_FUNCTION(GetWinners, 4);
		REGISTER_USER_FUNCTION(GetPlatformAccounting, 5);
		REGISTER_USER_FUNCTION(ValidateDigits, 6);
		REGISTER_USER_FUNCTION(GetPlayerTickets, 7);
		REGISTER_USER_FUNCTION(GetTemplateReadiness, 8);
		REGISTER_USER_FUNCTION(GetTemplates, 9);
	}

	/**
	 * @brief Initializes platform defaults when the contract state is first constructed.
	 * @note The QPI runtime zeroes the state before this procedure, so only non-zero defaults are assigned here.
	 */
	INITIALIZE()
	{
		state.mut().platformFeePercent = PULSEEDITOR_PLATFORM_FEE_PERCENT;
		state.mut().maxCreatorFeePercent = PULSEEDITOR_DEFAULT_MAX_CREATOR_FEE_PERCENT;
	}

	/**
	 * @brief Runs automated lifecycle work on a throttled tick cadence.
	 * @note The calendar-time guard mirrors Pulse: automation is skipped while QPI still exposes the bootstrap date.
	 */
	BEGIN_TICK_WITH_LOCALS()
	{
		if (mod(qpi.tick(), PULSEEDITOR_TICK_UPDATE_PERIOD) != 0)
		{
			return;
		}

		makeDateStamp(qpi.year(), qpi.month(), qpi.day(), locals.currentDateStamp);
		if (locals.currentDateStamp == PULSEEDITOR_DEFAULT_INIT_TIME)
		{
			return;
		}

		CALL(ProcessLifecycleAutomation, locals.automationInput, locals.automationOutput);
	}

	/**
	 * @brief Allows incoming asset management rights transfers into this contract.
	 * @param output Sets zero requested fee and allows the transfer.
	 * @note PulseEditor does not charge an acquire fee for managed assets in the MVP.
	 */
	PRE_ACQUIRE_SHARES()
	{
		output.requestedFee = 0;
		output.allowTransfer = true;
	}

	/**
	 * @brief Creates a draft fixed-payout game template owned by the invocator.
	 * @param input Template mechanics, ticket limits, fee settings, name, and payout matrix.
	 * @param output New template id and minimum required prize reserve.
	 * @return `SUCCESS`, `STORAGE_FULL`, or `INVALID_VALUE`.
	 * @note Any invocation reward is refunded because template creation is configuration-only.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(CreateTemplate)
	{
		if (qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		output.templateId = 0;
		output.requiredPrizeReserve = 0;
		output.requiredBonusReserve = 0;

		if (state.get().templateCount >= state.get().templates.capacity())
		{
			output.returnCode = toReturnCode(EReturnCode::STORAGE_FULL);
			return;
		}
		if (!isTemplateConfigValid(input.codeLength, input.maxDigit, input.ticketPrice, input.ticketLimit, input.playerTicketLimit,
		                           input.creatorFeePercent, input.burnPercent, state.get().maxCreatorFeePercent))
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_VALUE);
			return;
		}
		if (!isScheduleConfigValid(input.roundStartTick, input.roundEndTick))
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_VALUE);
			return;
		}
		if (!isRewardConfigValid(input.rewardMode, input.rewardAsset, input.rewardOwnershipManagingContractIndex,
		                         input.rewardPossessionManagingContractIndex))
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_VALUE);
			return;
		}
		if (!isEntryConfigValid(input.entryMode, input.entryAsset, input.rewardMode, input.rewardAsset, input.entryOwnershipManagingContractIndex,
		                        input.entryPossessionManagingContractIndex))
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_VALUE);
			return;
		}
		if (!isBonusConfigValid(input.bonusEnabled, input.bonusMultiplierBps, input.bonusAssetCount, input.bonusOwnershipManagingContractIndex,
		                        input.bonusPossessionManagingContractIndex))
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_VALUE);
			return;
		}
		for (locals.i = 0; locals.i < input.bonusAssetCount; ++locals.i)
		{
			if (input.bonusAssets.get(locals.i).assetName == 0 || input.bonusAssets.get(locals.i).issuer == NULL_ID)
			{
				output.returnCode = toReturnCode(EReturnCode::INVALID_VALUE);
				return;
			}
		}
		if (!input.allowRepeatedDigits && input.codeLength > static_cast<uint8>(input.maxDigit + 1))
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_VALUE);
			return;
		}

		locals.templateId = static_cast<uint16>(state.get().templateCount);
		locals.gameTemplate.name = input.name;
		locals.gameTemplate.payoutMatrix = input.payoutMatrix;
		locals.gameTemplate.bonusAssets = input.bonusAssets;
		locals.gameTemplate.owner = qpi.invocator();
		locals.gameTemplate.rewardAsset = input.rewardAsset;
		locals.gameTemplate.entryAsset = input.entryAsset;
		locals.gameTemplate.ticketPrice = input.ticketPrice;
		locals.gameTemplate.roundStartTick = input.roundStartTick;
		locals.gameTemplate.roundEndTick = input.roundEndTick;
		locals.gameTemplate.bonusMultiplierBps = input.bonusMultiplierBps;
		locals.gameTemplate.ticketLimit = input.ticketLimit;
		locals.gameTemplate.playerTicketLimit = input.playerTicketLimit;
		locals.gameTemplate.bonusAssetCount = input.bonusAssetCount;
		locals.gameTemplate.lastDrawEpoch = qpi.epoch();
		locals.gameTemplate.rewardOwnershipManagingContractIndex = input.rewardOwnershipManagingContractIndex;
		locals.gameTemplate.rewardPossessionManagingContractIndex = input.rewardPossessionManagingContractIndex;
		locals.gameTemplate.bonusOwnershipManagingContractIndex = input.bonusOwnershipManagingContractIndex;
		locals.gameTemplate.bonusPossessionManagingContractIndex = input.bonusPossessionManagingContractIndex;
		locals.gameTemplate.entryOwnershipManagingContractIndex = input.entryOwnershipManagingContractIndex;
		locals.gameTemplate.entryPossessionManagingContractIndex = input.entryPossessionManagingContractIndex;
		locals.gameTemplate.codeLength = input.codeLength;
		locals.gameTemplate.maxDigit = input.maxDigit;
		locals.gameTemplate.creatorFeePercent = input.creatorFeePercent;
		locals.gameTemplate.burnPercent = input.burnPercent;
		locals.gameTemplate.rewardMode = input.rewardMode;
		locals.gameTemplate.entryMode = input.entryMode;
		locals.gameTemplate.bonusEnabled = input.bonusEnabled;
		locals.gameTemplate.instantSettlement = input.instantSettlement;
		locals.gameTemplate.allowRepeatedDigits = input.allowRepeatedDigits;
		locals.gameTemplate.status = ETemplateStatus::DRAFT;
		locals.gameTemplate.maxSinglePayout = 0;

		for (locals.i = 0; locals.i < PULSEEDITOR_PAYOUT_MATRIX_SIZE; ++locals.i)
		{
			locals.payout = locals.gameTemplate.payoutMatrix.get(locals.i);
			if (locals.payout > locals.gameTemplate.maxSinglePayout)
			{
				locals.gameTemplate.maxSinglePayout = locals.payout;
			}
		}
		for (locals.i = PULSEEDITOR_PAYOUT_MATRIX_SIZE; locals.i < locals.gameTemplate.payoutMatrix.capacity(); ++locals.i)
		{
			locals.gameTemplate.payoutMatrix.set(locals.i, 0);
		}

		state.mut().templates.set(locals.templateId, locals.gameTemplate);
		state.mut().templateCount = sadd(state.get().templateCount, 1ULL);

		output.templateId = locals.templateId;
		output.requiredPrizeReserve = requiredBasePrizeReserve(locals.gameTemplate);
		output.requiredBonusReserve = requiredBonusReserve(locals.gameTemplate);
		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

	/**
	 * @brief Replaces the editable configuration of an unpublished draft template.
	 * @param input Template id and replacement mechanics/economic settings.
	 * @param output Recomputed reserve requirement and status code.
	 * @return `SUCCESS`, `INVALID_TEMPLATE`, `ACCESS_DENIED`, `INVALID_STATE`, or `INVALID_VALUE`.
	 * @note Existing prize reserve stays attached to the template; publication later enforces the new reserve requirement.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(UpdateTemplate)
	{
		if (qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}
		output.requiredPrizeReserve = 0;
		output.requiredBonusReserve = 0;

		if (!isTemplateIdValid(state, input.templateId))
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_TEMPLATE);
			return;
		}
		if (!isTemplateConfigValid(input.codeLength, input.maxDigit, input.ticketPrice, input.ticketLimit, input.playerTicketLimit,
		                           input.creatorFeePercent, input.burnPercent, state.get().maxCreatorFeePercent))
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_VALUE);
			return;
		}
		if (!isScheduleConfigValid(input.roundStartTick, input.roundEndTick))
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_VALUE);
			return;
		}
		if (!isRewardConfigValid(input.rewardMode, input.rewardAsset, input.rewardOwnershipManagingContractIndex,
		                         input.rewardPossessionManagingContractIndex))
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_VALUE);
			return;
		}
		if (!isEntryConfigValid(input.entryMode, input.entryAsset, input.rewardMode, input.rewardAsset, input.entryOwnershipManagingContractIndex,
		                        input.entryPossessionManagingContractIndex))
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_VALUE);
			return;
		}
		if (!isBonusConfigValid(input.bonusEnabled, input.bonusMultiplierBps, input.bonusAssetCount, input.bonusOwnershipManagingContractIndex,
		                        input.bonusPossessionManagingContractIndex))
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_VALUE);
			return;
		}
		for (locals.i = 0; locals.i < input.bonusAssetCount; ++locals.i)
		{
			if (input.bonusAssets.get(locals.i).assetName == 0 || input.bonusAssets.get(locals.i).issuer == NULL_ID)
			{
				output.returnCode = toReturnCode(EReturnCode::INVALID_VALUE);
				return;
			}
		}
		if (!input.allowRepeatedDigits && input.codeLength > static_cast<uint8>(input.maxDigit + 1))
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_VALUE);
			return;
		}

		locals.gameTemplate = state.get().templates.get(input.templateId);
		if (qpi.invocator() != locals.gameTemplate.owner)
		{
			output.returnCode = toReturnCode(EReturnCode::ACCESS_DENIED);
			return;
		}
		locals.round = state.get().rounds.get(input.templateId);
		if (locals.gameTemplate.hasTicketSales)
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_STATE);
			return;
		}
		if (locals.gameTemplate.status != ETemplateStatus::DRAFT &&
		    (locals.gameTemplate.status != ETemplateStatus::PUBLISHED || locals.round.status == ERoundStatus::SELLING))
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_STATE);
			return;
		}

		locals.gameTemplate.name = input.name;
		locals.gameTemplate.payoutMatrix = input.payoutMatrix;
		locals.gameTemplate.bonusAssets = input.bonusAssets;
		locals.gameTemplate.rewardAsset = input.rewardAsset;
		locals.gameTemplate.entryAsset = input.entryAsset;
		locals.gameTemplate.ticketPrice = input.ticketPrice;
		locals.gameTemplate.roundStartTick = input.roundStartTick;
		locals.gameTemplate.roundEndTick = input.roundEndTick;
		locals.gameTemplate.bonusMultiplierBps = input.bonusMultiplierBps;
		locals.gameTemplate.ticketLimit = input.ticketLimit;
		locals.gameTemplate.playerTicketLimit = input.playerTicketLimit;
		locals.gameTemplate.bonusAssetCount = input.bonusAssetCount;
		locals.gameTemplate.rewardOwnershipManagingContractIndex = input.rewardOwnershipManagingContractIndex;
		locals.gameTemplate.rewardPossessionManagingContractIndex = input.rewardPossessionManagingContractIndex;
		locals.gameTemplate.bonusOwnershipManagingContractIndex = input.bonusOwnershipManagingContractIndex;
		locals.gameTemplate.bonusPossessionManagingContractIndex = input.bonusPossessionManagingContractIndex;
		locals.gameTemplate.entryOwnershipManagingContractIndex = input.entryOwnershipManagingContractIndex;
		locals.gameTemplate.entryPossessionManagingContractIndex = input.entryPossessionManagingContractIndex;
		locals.gameTemplate.codeLength = input.codeLength;
		locals.gameTemplate.maxDigit = input.maxDigit;
		locals.gameTemplate.creatorFeePercent = input.creatorFeePercent;
		locals.gameTemplate.burnPercent = input.burnPercent;
		locals.gameTemplate.rewardMode = input.rewardMode;
		locals.gameTemplate.entryMode = input.entryMode;
		locals.gameTemplate.bonusEnabled = input.bonusEnabled;
		locals.gameTemplate.instantSettlement = input.instantSettlement;
		locals.gameTemplate.allowRepeatedDigits = input.allowRepeatedDigits;
		locals.gameTemplate.maxSinglePayout = 0;

		for (locals.i = 0; locals.i < PULSEEDITOR_PAYOUT_MATRIX_SIZE; ++locals.i)
		{
			locals.payout = locals.gameTemplate.payoutMatrix.get(locals.i);
			if (locals.payout > locals.gameTemplate.maxSinglePayout)
			{
				locals.gameTemplate.maxSinglePayout = locals.payout;
			}
		}
		for (locals.i = PULSEEDITOR_PAYOUT_MATRIX_SIZE; locals.i < locals.gameTemplate.payoutMatrix.capacity(); ++locals.i)
		{
			locals.gameTemplate.payoutMatrix.set(locals.i, 0);
		}

		state.mut().templates.set(input.templateId, locals.gameTemplate);

		output.requiredPrizeReserve = requiredBasePrizeReserve(locals.gameTemplate);
		output.requiredBonusReserve = requiredBonusReserve(locals.gameTemplate);
		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

	/**
	 * @brief Adds the attached Qubic invocation reward to a template prize reserve.
	 * @param input Target template id.
	 * @param output Deposited amount and updated reserve balance.
	 * @return `SUCCESS`, `INVALID_TEMPLATE`, `ACCESS_DENIED`, or `INSUFFICIENT_FUNDS`.
	 * @note Only the template owner may fund the reserve.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(DepositPrizeReserve)
	{
		output.depositedAmount = 0;
		output.prizeReserve = 0;

		if (!isTemplateIdValid(state, input.templateId))
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.returnCode = toReturnCode(EReturnCode::INVALID_TEMPLATE);
			return;
		}

		locals.gameTemplate = state.get().templates.get(input.templateId);
		if (qpi.invocator() != locals.gameTemplate.owner)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.returnCode = toReturnCode(EReturnCode::ACCESS_DENIED);
			return;
		}
		if (qpi.invocationReward() == 0)
		{
			output.returnCode = toReturnCode(EReturnCode::INSUFFICIENT_FUNDS);
			output.prizeReserve = locals.gameTemplate.prizeReserve;
			return;
		}

		locals.depositAmount = static_cast<uint64>(qpi.invocationReward());
		locals.gameTemplate.prizeReserve = sadd(locals.gameTemplate.prizeReserve, locals.depositAmount);
		state.mut().templates.set(input.templateId, locals.gameTemplate);

		output.depositedAmount = locals.depositAmount;
		output.prizeReserve = locals.gameTemplate.prizeReserve;
		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

	/**
	 * @brief Adds the attached Qubic invocation reward to a template multiplier-bonus reserve.
	 * @param input Target template id.
	 * @param output Deposited amount and updated bonus reserve balance.
	 * @return `SUCCESS`, `INVALID_TEMPLATE`, `ACCESS_DENIED`, `INVALID_STATE`, or `INSUFFICIENT_FUNDS`.
	 * @note Only the template owner may fund the bonus reserve, and the template must have multiplier bonuses enabled.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(DepositBonusReserve)
	{
		output.depositedAmount = 0;
		output.bonusReserve = 0;

		if (!isTemplateIdValid(state, input.templateId))
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.returnCode = toReturnCode(EReturnCode::INVALID_TEMPLATE);
			return;
		}

		locals.gameTemplate = state.get().templates.get(input.templateId);
		if (qpi.invocator() != locals.gameTemplate.owner)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.returnCode = toReturnCode(EReturnCode::ACCESS_DENIED);
			return;
		}
		if (!locals.gameTemplate.bonusEnabled || locals.gameTemplate.bonusMultiplierBps <= PULSEEDITOR_BONUS_MULTIPLIER_SCALE)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.returnCode = toReturnCode(EReturnCode::INVALID_STATE);
			output.bonusReserve = locals.gameTemplate.bonusReserve;
			return;
		}
		if (qpi.invocationReward() == 0)
		{
			output.returnCode = toReturnCode(EReturnCode::INSUFFICIENT_FUNDS);
			output.bonusReserve = locals.gameTemplate.bonusReserve;
			return;
		}

		locals.depositAmount = static_cast<uint64>(qpi.invocationReward());
		locals.gameTemplate.bonusReserve = sadd(locals.gameTemplate.bonusReserve, locals.depositAmount);
		state.mut().templates.set(input.templateId, locals.gameTemplate);

		output.depositedAmount = locals.depositAmount;
		output.bonusReserve = locals.gameTemplate.bonusReserve;
		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

	/**
	 * @brief Transfers reward asset shares from the template owner into base or bonus reserves.
	 * @param input Template id, share amount, and target reserve bucket.
	 * @param output Accepted share amount, updated asset reserves, and status code.
	 * @return `SUCCESS`, `INVALID_TEMPLATE`, `ACCESS_DENIED`, `INVALID_STATE`, or `INSUFFICIENT_FUNDS`.
	 * @note The invocator must possess the reward asset under this contract's management rights.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(DepositAssetReserve)
	{
		if (qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}
		output.depositedNumberOfShares = 0;
		output.assetPrizeReserve = 0;
		output.assetBonusReserve = 0;

		if (!isTemplateIdValid(state, input.templateId))
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_TEMPLATE);
			return;
		}

		locals.gameTemplate = state.get().templates.get(input.templateId);
		output.assetPrizeReserve = locals.gameTemplate.assetPrizeReserve;
		output.assetBonusReserve = locals.gameTemplate.assetBonusReserve;

		if (qpi.invocator() != locals.gameTemplate.owner)
		{
			output.returnCode = toReturnCode(EReturnCode::ACCESS_DENIED);
			return;
		}
		if (locals.gameTemplate.rewardMode != ERewardMode::ASSET || input.numberOfShares == 0 ||
		    input.numberOfShares > static_cast<uint64>(INT64_MAX))
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_STATE);
			return;
		}
		if (input.depositToBonusReserve &&
		    (!locals.gameTemplate.bonusEnabled || locals.gameTemplate.bonusMultiplierBps <= PULSEEDITOR_BONUS_MULTIPLIER_SCALE))
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_STATE);
			return;
		}

		locals.possessedShares = qpi.numberOfPossessedShares(
		    locals.gameTemplate.rewardAsset.assetName, locals.gameTemplate.rewardAsset.issuer, qpi.invocator(), qpi.invocator(),
		    locals.gameTemplate.rewardOwnershipManagingContractIndex, locals.gameTemplate.rewardPossessionManagingContractIndex);
		if (locals.possessedShares < static_cast<sint64>(input.numberOfShares))
		{
			output.returnCode = toReturnCode(EReturnCode::INSUFFICIENT_FUNDS);
			return;
		}

		locals.transferResult =
		    qpi.transferShareOwnershipAndPossession(locals.gameTemplate.rewardAsset.assetName, locals.gameTemplate.rewardAsset.issuer,
		                                            qpi.invocator(), qpi.invocator(), static_cast<sint64>(input.numberOfShares), SELF);
		if (locals.transferResult < 0)
		{
			output.returnCode = toReturnCode(EReturnCode::INSUFFICIENT_FUNDS);
			return;
		}

		if (input.depositToBonusReserve)
		{
			locals.gameTemplate.assetBonusReserve = sadd(locals.gameTemplate.assetBonusReserve, input.numberOfShares);
		}
		else
		{
			locals.gameTemplate.assetPrizeReserve = sadd(locals.gameTemplate.assetPrizeReserve, input.numberOfShares);
		}
		state.mut().templates.set(input.templateId, locals.gameTemplate);

		output.depositedNumberOfShares = input.numberOfShares;
		output.assetPrizeReserve = locals.gameTemplate.assetPrizeReserve;
		output.assetBonusReserve = locals.gameTemplate.assetBonusReserve;
		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

	/**
	 * @brief Publishes a funded draft template and opens its first selling round.
	 * @param input Template id to publish.
	 * @param output First round id.
	 * @return `SUCCESS`, `INVALID_TEMPLATE`, `ACCESS_DENIED`, `INVALID_STATE`, or `INSUFFICIENT_FUNDS`.
	 * @warning Publication requires the reserve required by `requiredBasePrizeReserve` plus any enabled bonus reserve.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(PublishTemplate)
	{
		if (qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if (!isTemplateIdValid(state, input.templateId))
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_TEMPLATE);
			return;
		}

		locals.gameTemplate = state.get().templates.get(input.templateId);
		if (qpi.invocator() != locals.gameTemplate.owner)
		{
			output.returnCode = toReturnCode(EReturnCode::ACCESS_DENIED);
			return;
		}
		if (locals.gameTemplate.status != ETemplateStatus::DRAFT)
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_STATE);
			return;
		}
		if (!hasRequiredBaseReserve(locals.gameTemplate))
		{
			output.returnCode = toReturnCode(EReturnCode::INSUFFICIENT_FUNDS);
			return;
		}
		if (locals.gameTemplate.bonusEnabled &&
		    ((locals.gameTemplate.rewardMode == ERewardMode::QUBIC && locals.gameTemplate.bonusReserve < requiredBonusReserve(locals.gameTemplate)) ||
		     (locals.gameTemplate.rewardMode == ERewardMode::ASSET &&
		      locals.gameTemplate.assetBonusReserve < requiredBonusReserve(locals.gameTemplate))))
		{
			output.returnCode = toReturnCode(EReturnCode::INSUFFICIENT_FUNDS);
			return;
		}
		if (locals.gameTemplate.roundEndTick > 0 && qpi.tick() > locals.gameTemplate.roundEndTick)
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_STATE);
			return;
		}

		locals.gameTemplate.status = ETemplateStatus::PUBLISHED;
		locals.gameTemplate.currentRoundId = sadd(locals.gameTemplate.currentRoundId, 1U);
		locals.gameTemplate.lastDrawEpoch = qpi.epoch();

		locals.round.roundId = locals.gameTemplate.currentRoundId;
		locals.round.startTick = locals.gameTemplate.roundStartTick;
		locals.round.endTick = locals.gameTemplate.roundEndTick;
		locals.round.status = ERoundStatus::SELLING;

		state.mut().templates.set(input.templateId, locals.gameTemplate);
		state.mut().rounds.set(input.templateId, locals.round);

		output.roundId = locals.round.roundId;
		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

	/**
	 * @brief Buys one ticket for a template's active selling round.
	 * @param input Template id and submitted digits.
	 * @param output Accepted round id, ticket index, and status code.
	 * @return `SUCCESS`, `INVALID_TEMPLATE`, `INVALID_STATE`, `TICKET_INVALID_PRICE`, `TICKET_SOLD_OUT`,
	 * `INVALID_DIGITS`, or `PLAYER_TICKET_LIMIT`.
	 * @note The invocation reward must exactly equal the ticket price; rejected purchases are refunded.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(BuyTicket)
	{
		output.ticketIndex = 0;
		output.roundId = 0;

		if (!isTemplateIdValid(state, input.templateId))
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.returnCode = toReturnCode(EReturnCode::INVALID_TEMPLATE);
			return;
		}

		locals.gameTemplate = state.get().templates.get(input.templateId);
		locals.round = state.get().rounds.get(input.templateId);
		locals.reward = qpi.invocationReward() > 0 ? static_cast<uint64>(qpi.invocationReward()) : 0;

		if (locals.gameTemplate.status != ETemplateStatus::PUBLISHED || locals.round.status != ERoundStatus::SELLING)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.returnCode = toReturnCode(EReturnCode::INVALID_STATE);
			return;
		}
		if (locals.round.startTick > 0 && qpi.tick() < locals.round.startTick)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.returnCode = toReturnCode(EReturnCode::INVALID_STATE);
			return;
		}
		if (locals.round.endTick > 0 && qpi.tick() > locals.round.endTick)
		{
			locals.round.status = ERoundStatus::CLOSED;
			state.mut().rounds.set(input.templateId, locals.round);
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.returnCode = toReturnCode(EReturnCode::INVALID_STATE);
			return;
		}
		if ((locals.gameTemplate.entryMode == EEntryMode::QUBIC && locals.reward != locals.gameTemplate.ticketPrice) ||
		    (locals.gameTemplate.entryMode == EEntryMode::ASSET && locals.reward != 0))
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.returnCode = toReturnCode(EReturnCode::TICKET_INVALID_PRICE);
			return;
		}
		if (state.get().ticketCount >= state.get().tickets.capacity() || locals.round.ticketCount >= locals.gameTemplate.ticketLimit)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.returnCode = toReturnCode(EReturnCode::TICKET_SOLD_OUT);
			return;
		}

		locals.validateInput.digits = input.digits;
		locals.validateInput.codeLength = locals.gameTemplate.codeLength;
		locals.validateInput.maxDigit = locals.gameTemplate.maxDigit;
		locals.validateInput.allowRepeatedDigits = locals.gameTemplate.allowRepeatedDigits;
		CALL(ValidateDigits, locals.validateInput, locals.validateOutput);
		if (!locals.validateOutput.isValid)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.returnCode = toReturnCode(EReturnCode::INVALID_DIGITS);
			return;
		}

		locals.playerTicketCount = 0;
		for (locals.i = 0; locals.i < state.get().ticketCount; ++locals.i)
		{
			locals.ticket = state.get().tickets.get(locals.i);
			if (locals.ticket.status == ETicketStatus::ACTIVE && locals.ticket.templateId == input.templateId &&
			    locals.ticket.roundId == locals.round.roundId && locals.ticket.player == qpi.invocator())
			{
				++locals.playerTicketCount;
			}
		}
		if (locals.playerTicketCount >= locals.gameTemplate.playerTicketLimit)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.returnCode = toReturnCode(EReturnCode::PLAYER_TICKET_LIMIT);
			return;
		}

		if (locals.gameTemplate.entryMode == EEntryMode::ASSET)
		{
			if (locals.gameTemplate.ticketPrice > static_cast<uint64>(INT64_MAX))
			{
				output.returnCode = toReturnCode(EReturnCode::TICKET_INVALID_PRICE);
				return;
			}
			locals.possessedShares = qpi.numberOfPossessedShares(
			    locals.gameTemplate.entryAsset.assetName, locals.gameTemplate.entryAsset.issuer, qpi.invocator(), qpi.invocator(),
			    locals.gameTemplate.entryOwnershipManagingContractIndex, locals.gameTemplate.entryPossessionManagingContractIndex);
			if (locals.possessedShares < static_cast<sint64>(locals.gameTemplate.ticketPrice))
			{
				output.returnCode = toReturnCode(EReturnCode::INSUFFICIENT_FUNDS);
				return;
			}
			locals.transferResult =
			    qpi.transferShareOwnershipAndPossession(locals.gameTemplate.entryAsset.assetName, locals.gameTemplate.entryAsset.issuer,
			                                            qpi.invocator(), qpi.invocator(), static_cast<sint64>(locals.gameTemplate.ticketPrice), SELF);
			if (locals.transferResult < 0)
			{
				output.returnCode = toReturnCode(EReturnCode::INSUFFICIENT_FUNDS);
				return;
			}
			locals.platformFee = div<uint64>(smul(locals.gameTemplate.ticketPrice, static_cast<uint64>(state.get().platformFeePercent)), 100ULL);
			locals.dev1Amount = div<uint64>(smul(locals.platformFee, static_cast<uint64>(PULSEEDITOR_PLATFORM_DEV1_SHARE_PERCENT)), 100ULL);
			locals.dev2Amount = div<uint64>(smul(locals.platformFee, static_cast<uint64>(PULSEEDITOR_PLATFORM_DEV2_SHARE_PERCENT)), 100ULL);
			locals.dividendAmount = locals.platformFee - locals.dev1Amount - locals.dev2Amount;
			locals.netRevenue = locals.gameTemplate.ticketPrice - locals.platformFee;
			locals.creatorAmount = div<uint64>(smul(locals.netRevenue, static_cast<uint64>(locals.gameTemplate.creatorFeePercent)), 100ULL);
			locals.burnAmount = div<uint64>(smul(locals.netRevenue, static_cast<uint64>(locals.gameTemplate.burnPercent)), 100ULL);
			locals.prizeAmount = locals.netRevenue - locals.creatorAmount - locals.burnAmount;
			if (locals.burnAmount > 0)
			{
				locals.transferResult =
				    qpi.transferShareOwnershipAndPossession(locals.gameTemplate.entryAsset.assetName, locals.gameTemplate.entryAsset.issuer, SELF,
				                                            SELF, static_cast<sint64>(locals.burnAmount), NULL_ID);
				if (locals.transferResult < 0)
				{
					qpi.transferShareOwnershipAndPossession(locals.gameTemplate.entryAsset.assetName, locals.gameTemplate.entryAsset.issuer, SELF,
					                                        SELF, static_cast<sint64>(locals.gameTemplate.ticketPrice), qpi.invocator());
					output.returnCode = toReturnCode(EReturnCode::INSUFFICIENT_FUNDS);
					return;
				}
			}
			locals.gameTemplate.assetDeveloper1Accrued = sadd(locals.gameTemplate.assetDeveloper1Accrued, locals.dev1Amount);
			locals.gameTemplate.assetDeveloper2Accrued = sadd(locals.gameTemplate.assetDeveloper2Accrued, locals.dev2Amount);
			locals.gameTemplate.assetDividendAccrued = sadd(locals.gameTemplate.assetDividendAccrued, locals.dividendAmount);
			locals.gameTemplate.assetCreatorRevenue = sadd(locals.gameTemplate.assetCreatorRevenue, locals.creatorAmount);
			locals.gameTemplate.assetBurnAccrued = sadd(locals.gameTemplate.assetBurnAccrued, locals.burnAmount);
			locals.gameTemplate.assetPrizeReserve = sadd(locals.gameTemplate.assetPrizeReserve, locals.prizeAmount);
			locals.gameTemplate.assetEntryRevenue = sadd(locals.gameTemplate.assetEntryRevenue, locals.gameTemplate.ticketPrice);
			locals.gameTemplate.hasTicketSales = true;
			locals.round.revenue = sadd(locals.round.revenue, locals.gameTemplate.ticketPrice);
			locals.round.prizeAdded = sadd(locals.round.prizeAdded, locals.prizeAmount);
		}
		else
		{
			locals.platformFee = div<uint64>(smul(locals.reward, static_cast<uint64>(state.get().platformFeePercent)), 100ULL);
			locals.dev1Amount = div<uint64>(smul(locals.platformFee, static_cast<uint64>(PULSEEDITOR_PLATFORM_DEV1_SHARE_PERCENT)), 100ULL);
			locals.dev2Amount = div<uint64>(smul(locals.platformFee, static_cast<uint64>(PULSEEDITOR_PLATFORM_DEV2_SHARE_PERCENT)), 100ULL);
			locals.dividendAmount = locals.platformFee - locals.dev1Amount - locals.dev2Amount;
			locals.netRevenue = locals.reward - locals.platformFee;
			locals.creatorAmount = div<uint64>(smul(locals.netRevenue, static_cast<uint64>(locals.gameTemplate.creatorFeePercent)), 100ULL);
			locals.burnAmount = div<uint64>(smul(locals.netRevenue, static_cast<uint64>(locals.gameTemplate.burnPercent)), 100ULL);
			locals.prizeAmount = locals.netRevenue - locals.creatorAmount - locals.burnAmount;

			state.mut().developer1Accrued = sadd(state.get().developer1Accrued, locals.dev1Amount);
			state.mut().developer2Accrued = sadd(state.get().developer2Accrued, locals.dev2Amount);
			state.mut().dividendAccrued = sadd(state.get().dividendAccrued, locals.dividendAmount);

			locals.gameTemplate.creatorRevenue = sadd(locals.gameTemplate.creatorRevenue, locals.creatorAmount);
			locals.gameTemplate.burnAccrued = sadd(locals.gameTemplate.burnAccrued, locals.burnAmount);
			if (locals.burnAmount > 0)
			{
				qpi.transfer(NULL_ID, locals.burnAmount);
			}
			locals.gameTemplate.prizeReserve = sadd(locals.gameTemplate.prizeReserve, locals.prizeAmount);
			locals.gameTemplate.totalRevenue = sadd(locals.gameTemplate.totalRevenue, locals.reward);
			locals.gameTemplate.hasTicketSales = true;
			locals.round.revenue = sadd(locals.round.revenue, locals.reward);
			locals.round.prizeAdded = sadd(locals.round.prizeAdded, locals.prizeAmount);
		}
		++locals.round.ticketCount;
		if (locals.round.ticketCount >= locals.gameTemplate.ticketLimit)
		{
			locals.round.status = ERoundStatus::CLOSED;
		}

		setMemory(locals.ticket, 0);
		locals.ticket.digits = input.digits;
		locals.ticket.player = qpi.invocator();
		locals.ticket.roundId = locals.round.roundId;
		locals.ticket.templateId = input.templateId;
		locals.ticket.status = ETicketStatus::ACTIVE;

		output.ticketIndex = state.get().ticketCount;
		output.roundId = locals.round.roundId;

		state.mut().tickets.set(state.get().ticketCount, locals.ticket);
		state.mut().ticketCount = sadd(state.get().ticketCount, 1ULL);
		state.mut().templates.set(input.templateId, locals.gameTemplate);
		state.mut().rounds.set(input.templateId, locals.round);

		if (locals.gameTemplate.instantSettlement)
		{
			locals.settleInput.templateId = input.templateId;
			locals.settleInput.ticketIndex = output.ticketIndex;
			CALL(SettleInstantTicket, locals.settleInput, locals.settleOutput);
			if (locals.settleOutput.returnCode != toReturnCode(EReturnCode::SUCCESS))
			{
				output.returnCode = locals.settleOutput.returnCode;
				return;
			}
		}

		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

	/**
	 * @brief Buys several tickets for the selected template.
	 * @param input Template id, ticket count, and submitted digit arrays.
	 * @param output First accepted round id, ticket indexes, accepted count, and status code.
	 * @return `SUCCESS`, `INVALID_TEMPLATE`, `INVALID_STATE`, `INVALID_VALUE`, `TICKET_INVALID_PRICE`,
	 * `TICKET_SOLD_OUT`, `INVALID_DIGITS`, `INSUFFICIENT_FUNDS`, or `PLAYER_TICKET_LIMIT`.
	 * @note Non-instant batches are stored atomically in one round; instant batches settle each accepted ticket sequentially.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(BuyTickets)
	{
		setMemory(output.ticketIndexes, 0);
		output.roundId = 0;
		output.acceptedCount = 0;

		if (!isTemplateIdValid(state, input.templateId))
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.returnCode = toReturnCode(EReturnCode::INVALID_TEMPLATE);
			return;
		}
		if (input.ticketCount == 0 || input.ticketCount > PULSEEDITOR_MAX_BATCH_TICKETS)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.returnCode = toReturnCode(EReturnCode::INVALID_VALUE);
			return;
		}

		locals.gameTemplate = state.get().templates.get(input.templateId);
		locals.round = state.get().rounds.get(input.templateId);
		locals.reward = qpi.invocationReward() > 0 ? static_cast<uint64>(qpi.invocationReward()) : 0;
		locals.totalPrice = smul(locals.gameTemplate.ticketPrice, static_cast<uint64>(input.ticketCount));

		if (locals.gameTemplate.status != ETemplateStatus::PUBLISHED || locals.round.status != ERoundStatus::SELLING)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.returnCode = toReturnCode(EReturnCode::INVALID_STATE);
			return;
		}
		if (locals.round.startTick > 0 && qpi.tick() < locals.round.startTick)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.returnCode = toReturnCode(EReturnCode::INVALID_STATE);
			return;
		}
		if (locals.round.endTick > 0 && qpi.tick() > locals.round.endTick)
		{
			locals.round.status = ERoundStatus::CLOSED;
			state.mut().rounds.set(input.templateId, locals.round);
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.returnCode = toReturnCode(EReturnCode::INVALID_STATE);
			return;
		}
		if ((locals.gameTemplate.entryMode == EEntryMode::QUBIC && locals.reward != locals.totalPrice) ||
		    (locals.gameTemplate.entryMode == EEntryMode::ASSET && locals.reward != 0))
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.returnCode = toReturnCode(EReturnCode::TICKET_INVALID_PRICE);
			return;
		}
		if (locals.gameTemplate.entryMode == EEntryMode::ASSET && locals.totalPrice > static_cast<uint64>(INT64_MAX))
		{
			output.returnCode = toReturnCode(EReturnCode::TICKET_INVALID_PRICE);
			return;
		}
		if (sadd(state.get().ticketCount, static_cast<uint64>(input.ticketCount)) > state.get().tickets.capacity())
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.returnCode = toReturnCode(EReturnCode::TICKET_SOLD_OUT);
			return;
		}
		if (!locals.gameTemplate.instantSettlement &&
		    sadd(static_cast<uint64>(locals.round.ticketCount), static_cast<uint64>(input.ticketCount)) > locals.gameTemplate.ticketLimit)
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.returnCode = toReturnCode(EReturnCode::TICKET_SOLD_OUT);
			return;
		}

		for (locals.i = 0; locals.i < input.ticketCount; ++locals.i)
		{
			locals.validateInput.digits = input.tickets.get(locals.i).digits;
			locals.validateInput.codeLength = locals.gameTemplate.codeLength;
			locals.validateInput.maxDigit = locals.gameTemplate.maxDigit;
			locals.validateInput.allowRepeatedDigits = locals.gameTemplate.allowRepeatedDigits;
			CALL(ValidateDigits, locals.validateInput, locals.validateOutput);
			if (!locals.validateOutput.isValid)
			{
				if (qpi.invocationReward() > 0)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				output.returnCode = toReturnCode(EReturnCode::INVALID_DIGITS);
				return;
			}
		}

		if (!locals.gameTemplate.instantSettlement)
		{
			locals.playerTicketCount = 0;
			for (locals.i = 0; locals.i < state.get().ticketCount; ++locals.i)
			{
				locals.ticket = state.get().tickets.get(locals.i);
				if (locals.ticket.status == ETicketStatus::ACTIVE && locals.ticket.templateId == input.templateId &&
				    locals.ticket.roundId == locals.round.roundId && locals.ticket.player == qpi.invocator())
				{
					++locals.playerTicketCount;
				}
			}
			if (sadd(static_cast<uint64>(locals.playerTicketCount), static_cast<uint64>(input.ticketCount)) > locals.gameTemplate.playerTicketLimit)
			{
				if (qpi.invocationReward() > 0)
				{
					qpi.transfer(qpi.invocator(), qpi.invocationReward());
				}
				output.returnCode = toReturnCode(EReturnCode::PLAYER_TICKET_LIMIT);
				return;
			}
			if (locals.gameTemplate.entryMode == EEntryMode::ASSET)
			{
				locals.possessedShares = qpi.numberOfPossessedShares(
				    locals.gameTemplate.entryAsset.assetName, locals.gameTemplate.entryAsset.issuer, qpi.invocator(), qpi.invocator(),
				    locals.gameTemplate.entryOwnershipManagingContractIndex, locals.gameTemplate.entryPossessionManagingContractIndex);
				if (locals.possessedShares < static_cast<sint64>(locals.totalPrice))
				{
					output.returnCode = toReturnCode(EReturnCode::INSUFFICIENT_FUNDS);
					return;
				}
				locals.transferResult =
				    qpi.transferShareOwnershipAndPossession(locals.gameTemplate.entryAsset.assetName, locals.gameTemplate.entryAsset.issuer,
				                                            qpi.invocator(), qpi.invocator(), static_cast<sint64>(locals.totalPrice), SELF);
				if (locals.transferResult < 0)
				{
					output.returnCode = toReturnCode(EReturnCode::INSUFFICIENT_FUNDS);
					return;
				}
			}
		}

		for (locals.i = 0; locals.i < input.ticketCount; ++locals.i)
		{
			if (locals.gameTemplate.instantSettlement)
			{
				locals.gameTemplate = state.get().templates.get(input.templateId);
				locals.round = state.get().rounds.get(input.templateId);
				if (locals.gameTemplate.status != ETemplateStatus::PUBLISHED || locals.round.status != ERoundStatus::SELLING)
				{
					if (locals.gameTemplate.entryMode == EEntryMode::QUBIC)
					{
						locals.totalPrice = smul(locals.gameTemplate.ticketPrice, static_cast<uint64>(input.ticketCount - output.acceptedCount));
						if (locals.totalPrice > 0)
						{
							qpi.transfer(qpi.invocator(), locals.totalPrice);
						}
					}
					output.returnCode = toReturnCode(EReturnCode::INVALID_STATE);
					return;
				}
				if (locals.round.startTick > 0 && qpi.tick() < locals.round.startTick)
				{
					if (locals.gameTemplate.entryMode == EEntryMode::QUBIC)
					{
						locals.totalPrice = smul(locals.gameTemplate.ticketPrice, static_cast<uint64>(input.ticketCount - output.acceptedCount));
						if (locals.totalPrice > 0)
						{
							qpi.transfer(qpi.invocator(), locals.totalPrice);
						}
					}
					output.returnCode = toReturnCode(EReturnCode::INVALID_STATE);
					return;
				}
				if (locals.round.endTick > 0 && qpi.tick() > locals.round.endTick)
				{
					locals.round.status = ERoundStatus::CLOSED;
					state.mut().rounds.set(input.templateId, locals.round);
					if (locals.gameTemplate.entryMode == EEntryMode::QUBIC)
					{
						locals.totalPrice = smul(locals.gameTemplate.ticketPrice, static_cast<uint64>(input.ticketCount - output.acceptedCount));
						if (locals.totalPrice > 0)
						{
							qpi.transfer(qpi.invocator(), locals.totalPrice);
						}
					}
					output.returnCode = toReturnCode(EReturnCode::INVALID_STATE);
					return;
				}
				if (state.get().ticketCount >= state.get().tickets.capacity() || locals.round.ticketCount >= locals.gameTemplate.ticketLimit)
				{
					if (locals.gameTemplate.entryMode == EEntryMode::QUBIC)
					{
						locals.totalPrice = smul(locals.gameTemplate.ticketPrice, static_cast<uint64>(input.ticketCount - output.acceptedCount));
						if (locals.totalPrice > 0)
						{
							qpi.transfer(qpi.invocator(), locals.totalPrice);
						}
					}
					output.returnCode = toReturnCode(EReturnCode::TICKET_SOLD_OUT);
					return;
				}
			}

			if (locals.gameTemplate.entryMode == EEntryMode::ASSET)
			{
				if (locals.gameTemplate.instantSettlement)
				{
					locals.possessedShares = qpi.numberOfPossessedShares(
					    locals.gameTemplate.entryAsset.assetName, locals.gameTemplate.entryAsset.issuer, qpi.invocator(), qpi.invocator(),
					    locals.gameTemplate.entryOwnershipManagingContractIndex, locals.gameTemplate.entryPossessionManagingContractIndex);
					if (locals.possessedShares < static_cast<sint64>(locals.gameTemplate.ticketPrice))
					{
						output.returnCode = toReturnCode(EReturnCode::INSUFFICIENT_FUNDS);
						return;
					}
					locals.transferResult = qpi.transferShareOwnershipAndPossession(
					    locals.gameTemplate.entryAsset.assetName, locals.gameTemplate.entryAsset.issuer, qpi.invocator(), qpi.invocator(),
					    static_cast<sint64>(locals.gameTemplate.ticketPrice), SELF);
					if (locals.transferResult < 0)
					{
						output.returnCode = toReturnCode(EReturnCode::INSUFFICIENT_FUNDS);
						return;
					}
				}
				locals.platformFee = div<uint64>(smul(locals.gameTemplate.ticketPrice, static_cast<uint64>(state.get().platformFeePercent)), 100ULL);
				locals.dev1Amount = div<uint64>(smul(locals.platformFee, static_cast<uint64>(PULSEEDITOR_PLATFORM_DEV1_SHARE_PERCENT)), 100ULL);
				locals.dev2Amount = div<uint64>(smul(locals.platformFee, static_cast<uint64>(PULSEEDITOR_PLATFORM_DEV2_SHARE_PERCENT)), 100ULL);
				locals.dividendAmount = locals.platformFee - locals.dev1Amount - locals.dev2Amount;
				locals.netRevenue = locals.gameTemplate.ticketPrice - locals.platformFee;
				locals.creatorAmount = div<uint64>(smul(locals.netRevenue, static_cast<uint64>(locals.gameTemplate.creatorFeePercent)), 100ULL);
				locals.burnAmount = div<uint64>(smul(locals.netRevenue, static_cast<uint64>(locals.gameTemplate.burnPercent)), 100ULL);
				locals.prizeAmount = locals.netRevenue - locals.creatorAmount - locals.burnAmount;
				if (locals.burnAmount > 0)
				{
					locals.transferResult =
					    qpi.transferShareOwnershipAndPossession(locals.gameTemplate.entryAsset.assetName, locals.gameTemplate.entryAsset.issuer, SELF,
					                                            SELF, static_cast<sint64>(locals.burnAmount), NULL_ID);
					if (locals.transferResult < 0)
					{
						if (locals.gameTemplate.instantSettlement)
						{
							qpi.transferShareOwnershipAndPossession(locals.gameTemplate.entryAsset.assetName, locals.gameTemplate.entryAsset.issuer,
							                                        SELF, SELF, static_cast<sint64>(locals.gameTemplate.ticketPrice),
							                                        qpi.invocator());
						}
						else if (output.acceptedCount == 0)
						{
							qpi.transferShareOwnershipAndPossession(locals.gameTemplate.entryAsset.assetName, locals.gameTemplate.entryAsset.issuer,
							                                        SELF, SELF, static_cast<sint64>(locals.totalPrice), qpi.invocator());
						}
						output.returnCode = toReturnCode(EReturnCode::INSUFFICIENT_FUNDS);
						return;
					}
				}
				locals.gameTemplate.assetDeveloper1Accrued = sadd(locals.gameTemplate.assetDeveloper1Accrued, locals.dev1Amount);
				locals.gameTemplate.assetDeveloper2Accrued = sadd(locals.gameTemplate.assetDeveloper2Accrued, locals.dev2Amount);
				locals.gameTemplate.assetDividendAccrued = sadd(locals.gameTemplate.assetDividendAccrued, locals.dividendAmount);
				locals.gameTemplate.assetCreatorRevenue = sadd(locals.gameTemplate.assetCreatorRevenue, locals.creatorAmount);
				locals.gameTemplate.assetBurnAccrued = sadd(locals.gameTemplate.assetBurnAccrued, locals.burnAmount);
				locals.gameTemplate.assetPrizeReserve = sadd(locals.gameTemplate.assetPrizeReserve, locals.prizeAmount);
				locals.gameTemplate.assetEntryRevenue = sadd(locals.gameTemplate.assetEntryRevenue, locals.gameTemplate.ticketPrice);
				locals.gameTemplate.hasTicketSales = true;
				locals.round.revenue = sadd(locals.round.revenue, locals.gameTemplate.ticketPrice);
				locals.round.prizeAdded = sadd(locals.round.prizeAdded, locals.prizeAmount);
			}
			else
			{
				locals.platformFee = div<uint64>(smul(locals.gameTemplate.ticketPrice, static_cast<uint64>(state.get().platformFeePercent)), 100ULL);
				locals.dev1Amount = div<uint64>(smul(locals.platformFee, static_cast<uint64>(PULSEEDITOR_PLATFORM_DEV1_SHARE_PERCENT)), 100ULL);
				locals.dev2Amount = div<uint64>(smul(locals.platformFee, static_cast<uint64>(PULSEEDITOR_PLATFORM_DEV2_SHARE_PERCENT)), 100ULL);
				locals.dividendAmount = locals.platformFee - locals.dev1Amount - locals.dev2Amount;
				locals.netRevenue = locals.gameTemplate.ticketPrice - locals.platformFee;
				locals.creatorAmount = div<uint64>(smul(locals.netRevenue, static_cast<uint64>(locals.gameTemplate.creatorFeePercent)), 100ULL);
				locals.burnAmount = div<uint64>(smul(locals.netRevenue, static_cast<uint64>(locals.gameTemplate.burnPercent)), 100ULL);
				locals.prizeAmount = locals.netRevenue - locals.creatorAmount - locals.burnAmount;

				state.mut().developer1Accrued = sadd(state.get().developer1Accrued, locals.dev1Amount);
				state.mut().developer2Accrued = sadd(state.get().developer2Accrued, locals.dev2Amount);
				state.mut().dividendAccrued = sadd(state.get().dividendAccrued, locals.dividendAmount);

				locals.gameTemplate.creatorRevenue = sadd(locals.gameTemplate.creatorRevenue, locals.creatorAmount);
				locals.gameTemplate.burnAccrued = sadd(locals.gameTemplate.burnAccrued, locals.burnAmount);
				if (locals.burnAmount > 0)
				{
					qpi.transfer(NULL_ID, locals.burnAmount);
				}
				locals.gameTemplate.prizeReserve = sadd(locals.gameTemplate.prizeReserve, locals.prizeAmount);
				locals.gameTemplate.totalRevenue = sadd(locals.gameTemplate.totalRevenue, locals.gameTemplate.ticketPrice);
				locals.gameTemplate.hasTicketSales = true;
				locals.round.revenue = sadd(locals.round.revenue, locals.gameTemplate.ticketPrice);
				locals.round.prizeAdded = sadd(locals.round.prizeAdded, locals.prizeAmount);
			}
			++locals.round.ticketCount;

			setMemory(locals.ticket, 0);
			locals.ticket.digits = input.tickets.get(locals.i).digits;
			locals.ticket.player = qpi.invocator();
			locals.ticket.roundId = locals.round.roundId;
			locals.ticket.templateId = input.templateId;
			locals.ticket.status = ETicketStatus::ACTIVE;

			if (output.acceptedCount == 0)
			{
				output.roundId = locals.round.roundId;
			}
			output.ticketIndexes.set(locals.i, state.get().ticketCount);
			state.mut().tickets.set(state.get().ticketCount, locals.ticket);
			state.mut().ticketCount = sadd(state.get().ticketCount, 1ULL);
			++output.acceptedCount;

			if (locals.round.ticketCount >= locals.gameTemplate.ticketLimit)
			{
				locals.round.status = ERoundStatus::CLOSED;
			}
			state.mut().templates.set(input.templateId, locals.gameTemplate);
			state.mut().rounds.set(input.templateId, locals.round);

			if (locals.gameTemplate.instantSettlement)
			{
				locals.settleInput.templateId = input.templateId;
				locals.settleInput.ticketIndex = output.ticketIndexes.get(locals.i);
				CALL(SettleInstantTicket, locals.settleInput, locals.settleOutput);
				if (locals.settleOutput.returnCode != toReturnCode(EReturnCode::SUCCESS))
				{
					if (locals.gameTemplate.entryMode == EEntryMode::QUBIC)
					{
						locals.totalPrice = smul(locals.gameTemplate.ticketPrice, static_cast<uint64>(input.ticketCount - output.acceptedCount));
						if (locals.totalPrice > 0)
						{
							qpi.transfer(qpi.invocator(), locals.totalPrice);
						}
					}
					output.returnCode = locals.settleOutput.returnCode;
					return;
				}
			}
		}

		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

	/**
	 * @brief Settles an eligible active round from tick automation.
	 * @param input Template id whose current round should be settled.
	 * @param output Winning digits, paid total, winner counts, and status code.
	 * @return `SUCCESS`, `INVALID_TEMPLATE`, or `INVALID_STATE`.
	 * @note This is not user-callable; external callers cannot force draws outside the tick lifecycle.
	 * @warning Winners are marked unpaid if the prize reserve cannot cover their fixed payout.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(SettleRound)
	{
		if (!isTemplateIdValid(state, input.templateId))
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_TEMPLATE);
			return;
		}

		locals.gameTemplate = state.get().templates.get(input.templateId);
		locals.round = state.get().rounds.get(input.templateId);

		if (locals.round.status != ERoundStatus::SELLING && locals.round.status != ERoundStatus::CLOSED)
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_STATE);
			return;
		}
		if (locals.round.status == ERoundStatus::SELLING && locals.round.ticketCount < locals.gameTemplate.ticketLimit && locals.round.endTick > 0 &&
		    qpi.tick() <= locals.round.endTick)
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_STATE);
			return;
		}
		if (locals.round.status == ERoundStatus::SELLING)
		{
			locals.round.status = ERoundStatus::CLOSED;
		}

		locals.randomData.prevSpectrumDigest = qpi.getPrevSpectrumDigest();
		locals.randomData.templateId = input.templateId;
		locals.randomData.roundId = locals.round.roundId;
		locals.randomData.ticketCount = locals.round.ticketCount;
		locals.seed = qpi.K12(locals.randomData).u64._0;
		locals.generateInput.seed = locals.seed;
		locals.generateInput.codeLength = locals.gameTemplate.codeLength;
		locals.generateInput.maxDigit = locals.gameTemplate.maxDigit;
		locals.generateInput.allowRepeatedDigits = locals.gameTemplate.allowRepeatedDigits;
		CALL(GenerateWinningDigits, locals.generateInput, locals.generateOutput);

		locals.round.winningDigits = locals.generateOutput.digits;
		for (locals.i = 0; locals.i < state.get().ticketCount; ++locals.i)
		{
			locals.ticket = state.get().tickets.get(locals.i);
			if (locals.ticket.status != ETicketStatus::ACTIVE || locals.ticket.templateId != input.templateId ||
			    locals.ticket.roundId != locals.round.roundId)
			{
				continue;
			}

			locals.countInput.playerDigits = locals.ticket.digits;
			locals.countInput.winningDigits = locals.round.winningDigits;
			locals.countInput.codeLength = locals.gameTemplate.codeLength;
			CALL(CountMatches, locals.countInput, locals.countOutput);

			locals.ticket.exact = locals.countOutput.exact;
			locals.ticket.misplaced = locals.countOutput.misplaced;
			locals.payout = locals.gameTemplate.payoutMatrix.get(locals.countOutput.payoutMatrixIndex);
			locals.bonusPayout = 0;
			locals.bonusQualified = false;

			if (locals.payout > 0)
			{
				++locals.round.winnerCount;
				if (locals.gameTemplate.bonusEnabled && locals.gameTemplate.bonusMultiplierBps > PULSEEDITOR_BONUS_MULTIPLIER_SCALE)
				{
					for (locals.bonusAssetIndex = 0; locals.bonusAssetIndex < locals.gameTemplate.bonusAssetCount; ++locals.bonusAssetIndex)
					{
						locals.bonusShares = qpi.numberOfPossessedShares(
						    locals.gameTemplate.bonusAssets.get(locals.bonusAssetIndex).assetName,
						    locals.gameTemplate.bonusAssets.get(locals.bonusAssetIndex).issuer, locals.ticket.player, locals.ticket.player,
						    locals.gameTemplate.bonusOwnershipManagingContractIndex, locals.gameTemplate.bonusPossessionManagingContractIndex);
						if (locals.bonusShares > 0)
						{
							locals.bonusQualified = true;
							break;
						}
					}
					locals.bonusPayout = bonusExtraPayout(locals.payout, locals.gameTemplate.bonusMultiplierBps);
					if (!locals.bonusQualified || !((locals.gameTemplate.rewardMode == ERewardMode::QUBIC &&
					                                 locals.gameTemplate.bonusReserve >= locals.bonusPayout) ||
					                                (locals.gameTemplate.rewardMode == ERewardMode::ASSET &&
					                                 locals.gameTemplate.assetBonusReserve >= locals.bonusPayout)))
					{
						locals.bonusPayout = 0;
					}
				}
				locals.totalPayout = sadd(locals.payout, locals.bonusPayout);
				locals.ticket.payout = locals.gameTemplate.rewardMode == ERewardMode::QUBIC ? locals.totalPayout : 0;
				locals.ticket.bonusPayout = locals.gameTemplate.rewardMode == ERewardMode::QUBIC ? locals.bonusPayout : 0;
				locals.ticket.assetPayout = locals.gameTemplate.rewardMode == ERewardMode::ASSET ? locals.totalPayout : 0;
				locals.ticket.assetBonusPayout = locals.gameTemplate.rewardMode == ERewardMode::ASSET ? locals.bonusPayout : 0;
				if (locals.gameTemplate.rewardMode == ERewardMode::QUBIC && locals.gameTemplate.prizeReserve >= locals.payout)
				{
					qpi.transfer(locals.ticket.player, locals.totalPayout);
					locals.gameTemplate.prizeReserve = locals.gameTemplate.prizeReserve - locals.payout;
					locals.gameTemplate.bonusReserve = locals.gameTemplate.bonusReserve - locals.bonusPayout;
					locals.gameTemplate.totalPaid = sadd(locals.gameTemplate.totalPaid, locals.totalPayout);
					locals.gameTemplate.totalBonusPaid = sadd(locals.gameTemplate.totalBonusPaid, locals.bonusPayout);
					locals.round.paid = sadd(locals.round.paid, locals.totalPayout);
					locals.ticket.status = ETicketStatus::PAID;
				}
				else if (locals.gameTemplate.rewardMode == ERewardMode::ASSET && locals.gameTemplate.assetPrizeReserve >= locals.payout &&
				         locals.totalPayout <= static_cast<uint64>(INT64_MAX))
				{
					locals.transferResult =
					    qpi.transferShareOwnershipAndPossession(locals.gameTemplate.rewardAsset.assetName, locals.gameTemplate.rewardAsset.issuer,
					                                            SELF, SELF, static_cast<sint64>(locals.totalPayout), locals.ticket.player);
					if (locals.transferResult < 0)
					{
						++locals.round.unpaidWinnerCount;
						locals.ticket.status = ETicketStatus::UNPAID;
					}
					else
					{
						locals.gameTemplate.assetPrizeReserve = locals.gameTemplate.assetPrizeReserve - locals.payout;
						locals.gameTemplate.assetBonusReserve = locals.gameTemplate.assetBonusReserve - locals.bonusPayout;
						locals.gameTemplate.totalAssetPaid = sadd(locals.gameTemplate.totalAssetPaid, locals.totalPayout);
						locals.gameTemplate.totalBonusPaid = sadd(locals.gameTemplate.totalBonusPaid, locals.bonusPayout);
						locals.round.paid = sadd(locals.round.paid, locals.totalPayout);
						locals.ticket.status = ETicketStatus::PAID;
					}
				}
				else
				{
					++locals.round.unpaidWinnerCount;
					locals.ticket.status = ETicketStatus::UNPAID;
				}

				if (locals.ticket.status == ETicketStatus::PAID)
				{
					locals.winnerInfo.epoch = qpi.epoch();
					locals.winnerInfo.tick = qpi.tick();
					locals.winnerInfo.player = locals.ticket.player;
					locals.winnerInfo.payout = locals.ticket.payout;
					locals.winnerInfo.bonusPayout = locals.ticket.bonusPayout;
					locals.winnerInfo.assetPayout = locals.ticket.assetPayout;
					locals.winnerInfo.assetBonusPayout = locals.ticket.assetBonusPayout;
					locals.winnerInfo.roundId = locals.round.roundId;
					locals.winnerInfo.templateId = input.templateId;
					locals.winnerInfo.exact = locals.ticket.exact;
					locals.winnerInfo.misplaced = locals.ticket.misplaced;
					locals.winnerIndex = mod(state.get().winnerCounter, state.get().winners.capacity());
					state.mut().winners.set(locals.winnerIndex, locals.winnerInfo);
					state.mut().winnerCounter = sadd(state.get().winnerCounter, 1ULL);
				}
			}
			else
			{
				locals.ticket.payout = 0;
				locals.ticket.bonusPayout = 0;
				locals.ticket.assetPayout = 0;
				locals.ticket.assetBonusPayout = 0;
				locals.ticket.status = ETicketStatus::PAID;
			}

			state.mut().tickets.set(locals.i, locals.ticket);
		}

		locals.round.settledTick = qpi.tick();
		locals.round.status = ERoundStatus::SETTLED;
		locals.gameTemplate.lastDrawEpoch = qpi.epoch();
		if (locals.gameTemplate.status == ETemplateStatus::STOP_REQUESTED)
		{
			locals.gameTemplate.status = ETemplateStatus::STOPPED;
		}

		state.mut().templates.set(input.templateId, locals.gameTemplate);
		state.mut().rounds.set(input.templateId, locals.round);

		output.winningDigits = locals.round.winningDigits;
		output.totalPaid = locals.round.paid;
		output.winnerCount = locals.round.winnerCount;
		output.unpaidWinnerCount = locals.round.unpaidWinnerCount;
		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

	/**
	 * @brief Requests graceful template shutdown.
	 * @param input Template id owned by the invocator.
	 * @param output Status code.
	 * @return `SUCCESS`, `INVALID_TEMPLATE`, or `ACCESS_DENIED`.
	 * @note If a round is still selling, the template enters `STOP_REQUESTED` and becomes stopped after settlement.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(RequestStop)
	{
		if (qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if (!isTemplateIdValid(state, input.templateId))
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_TEMPLATE);
			return;
		}

		locals.gameTemplate = state.get().templates.get(input.templateId);
		locals.round = state.get().rounds.get(input.templateId);
		if (qpi.invocator() != locals.gameTemplate.owner)
		{
			output.returnCode = toReturnCode(EReturnCode::ACCESS_DENIED);
			return;
		}
		if (locals.round.status == ERoundStatus::SELLING || locals.round.status == ERoundStatus::CLOSED)
		{
			locals.gameTemplate.status = ETemplateStatus::STOP_REQUESTED;
		}
		else
		{
			locals.gameTemplate.status = ETemplateStatus::STOPPED;
		}

		state.mut().templates.set(input.templateId, locals.gameTemplate);
		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

	/**
	 * @brief Withdraws creator revenue accrued from ticket purchases.
	 * @param input Template id and amount to withdraw.
	 * @param output Withdrawn amount, remaining creator revenue, and status code.
	 * @return `SUCCESS`, `INVALID_TEMPLATE`, `ACCESS_DENIED`, or `INSUFFICIENT_FUNDS`.
	 * @note Prize reserves and platform accounting are not affected by this withdrawal.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(WithdrawCreatorRevenue)
	{
		if (!isTemplateIdValid(state, input.templateId))
		{
			if (qpi.invocationReward() > 0)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			output.returnCode = toReturnCode(EReturnCode::INVALID_TEMPLATE);
			return;
		}
		if (qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		locals.gameTemplate = state.get().templates.get(input.templateId);
		if (qpi.invocator() != locals.gameTemplate.owner)
		{
			output.returnCode = toReturnCode(EReturnCode::ACCESS_DENIED);
			return;
		}
		if (locals.gameTemplate.entryMode == EEntryMode::ASSET)
		{
			if (input.amount == 0 || input.amount > locals.gameTemplate.assetCreatorRevenue || input.amount > static_cast<uint64>(INT64_MAX))
			{
				output.returnCode = toReturnCode(EReturnCode::INSUFFICIENT_FUNDS);
				output.remainingRevenue = locals.gameTemplate.assetCreatorRevenue;
				return;
			}

			locals.amount = input.amount;
			locals.transferResult =
			    qpi.transferShareOwnershipAndPossession(locals.gameTemplate.entryAsset.assetName, locals.gameTemplate.entryAsset.issuer, SELF, SELF,
			                                            static_cast<sint64>(locals.amount), qpi.invocator());
			if (locals.transferResult < 0)
			{
				output.returnCode = toReturnCode(EReturnCode::INSUFFICIENT_FUNDS);
				output.remainingRevenue = locals.gameTemplate.assetCreatorRevenue;
				return;
			}
			locals.gameTemplate.assetCreatorRevenue = locals.gameTemplate.assetCreatorRevenue - locals.amount;
			state.mut().templates.set(input.templateId, locals.gameTemplate);

			output.withdrawnAmount = locals.amount;
			output.remainingRevenue = locals.gameTemplate.assetCreatorRevenue;
			output.returnCode = toReturnCode(EReturnCode::SUCCESS);
			return;
		}

		if (input.amount == 0 || input.amount > locals.gameTemplate.creatorRevenue)
		{
			output.returnCode = toReturnCode(EReturnCode::INSUFFICIENT_FUNDS);
			output.remainingRevenue = locals.gameTemplate.creatorRevenue;
			return;
		}

		locals.amount = input.amount;
		qpi.transfer(qpi.invocator(), locals.amount);
		locals.gameTemplate.creatorRevenue = locals.gameTemplate.creatorRevenue - locals.amount;
		state.mut().templates.set(input.templateId, locals.gameTemplate);

		output.withdrawnAmount = locals.amount;
		output.remainingRevenue = locals.gameTemplate.creatorRevenue;
		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

	/**
	 * @brief Initializes or updates platform owner, fee recipients, and creator fee limit.
	 * @param input Platform owner, developer recipients, and maximum creator fee.
	 * @param output Status code.
	 * @return `SUCCESS`, `ACCESS_DENIED`, or `INVALID_VALUE`.
	 * @note The first successful call may bootstrap the platform owner when no owner is configured.
	 */
	PUBLIC_PROCEDURE(SetPlatformConfig)
	{
		if (qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if (state.get().platformOwner != NULL_ID && qpi.invocator() != state.get().platformOwner)
		{
			output.returnCode = toReturnCode(EReturnCode::ACCESS_DENIED);
			return;
		}
		if (input.platformOwner == NULL_ID || input.developer1 == NULL_ID || input.developer2 == NULL_ID || input.maxCreatorFeePercent > 100)
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_VALUE);
			return;
		}

		state.mut().platformOwner = input.platformOwner;
		state.mut().developer1 = input.developer1;
		state.mut().developer2 = input.developer2;
		state.mut().maxCreatorFeePercent = input.maxCreatorFeePercent;
		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

	/**
	 * @brief Withdraws accumulated platform fee balances to developers and distributes shareholder dividends.
	 * @param input Empty input.
	 * @param output Amounts transferred/distributed to each platform revenue bucket and status code.
	 * @return `SUCCESS`, `ACCESS_DENIED`, or `INVALID_VALUE`.
	 * @note Dividends are paid from this SC balance with `qpi.distributeDividends(amountPerShare)`.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(WithdrawPlatformRevenue)
	{
		if (qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}

		if (qpi.invocator() != state.get().platformOwner)
		{
			output.returnCode = toReturnCode(EReturnCode::ACCESS_DENIED);
			return;
		}
		if (state.get().developer1 == NULL_ID || state.get().developer2 == NULL_ID)
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_VALUE);
			return;
		}

		locals.developer1Amount = state.get().developer1Accrued;
		locals.developer2Amount = state.get().developer2Accrued;
		locals.dividendPerShare = div<uint64>(state.get().dividendAccrued, NUMBER_OF_COMPUTORS);
		locals.dividendAmount = smul(locals.dividendPerShare, static_cast<uint64>(NUMBER_OF_COMPUTORS));

		if (locals.dividendPerShare > 0 && !qpi.distributeDividends(static_cast<sint64>(locals.dividendPerShare)))
		{
			output.returnCode = toReturnCode(EReturnCode::INSUFFICIENT_FUNDS);
			return;
		}

		if (locals.developer1Amount > 0)
		{
			qpi.transfer(state.get().developer1, locals.developer1Amount);
		}
		if (locals.developer2Amount > 0)
		{
			qpi.transfer(state.get().developer2, locals.developer2Amount);
		}

		state.mut().developer1Accrued = 0;
		state.mut().developer2Accrued = 0;
		state.mut().dividendAccrued = state.get().dividendAccrued - locals.dividendAmount;

		output.developer1Amount = locals.developer1Amount;
		output.developer2Amount = locals.developer2Amount;
		output.dividendAmount = locals.dividendAmount;
		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

	/**
	 * @brief Withdraws asset-denominated platform developer fees for one asset-entry template.
	 * @param input Template id whose asset platform balances should be processed.
	 * @param output Developer transfer amounts, distributed dividend amount, retained dividend balance, and status code.
	 * @return `SUCCESS`, `INVALID_TEMPLATE`, `ACCESS_DENIED`, `INVALID_STATE`, `INVALID_VALUE`, or `INSUFFICIENT_FUNDS`.
	 * @note Asset dividends are distributed to holders of the PulseEditor contract-share asset.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(WithdrawAssetPlatformRevenue)
	{
		if (qpi.invocationReward() > 0)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}
		output.developer1Amount = 0;
		output.developer2Amount = 0;
		output.dividendAmount = 0;
		output.dividendAccrued = 0;

		if (!isTemplateIdValid(state, input.templateId))
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_TEMPLATE);
			return;
		}
		if (qpi.invocator() != state.get().platformOwner)
		{
			output.returnCode = toReturnCode(EReturnCode::ACCESS_DENIED);
			return;
		}
		if (state.get().developer1 == NULL_ID || state.get().developer2 == NULL_ID)
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_VALUE);
			return;
		}

		locals.gameTemplate = state.get().templates.get(input.templateId);
		output.dividendAccrued = locals.gameTemplate.assetDividendAccrued;
		if (locals.gameTemplate.entryMode != EEntryMode::ASSET || locals.gameTemplate.rewardMode != ERewardMode::ASSET)
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_STATE);
			return;
		}

		locals.developer1Amount = locals.gameTemplate.assetDeveloper1Accrued;
		locals.developer2Amount = locals.gameTemplate.assetDeveloper2Accrued;
		locals.dividendAmount = div<uint64>(locals.gameTemplate.assetDividendAccrued, static_cast<uint64>(NUMBER_OF_COMPUTORS)) *
		                        static_cast<uint64>(NUMBER_OF_COMPUTORS);
		locals.totalAmount = sadd(sadd(locals.developer1Amount, locals.developer2Amount), locals.dividendAmount);
		if (locals.totalAmount > static_cast<uint64>(INT64_MAX))
		{
			output.returnCode = toReturnCode(EReturnCode::INSUFFICIENT_FUNDS);
			return;
		}
		locals.possessedShares = qpi.numberOfPossessedShares(locals.gameTemplate.entryAsset.assetName, locals.gameTemplate.entryAsset.issuer, SELF,
		                                                     SELF, locals.gameTemplate.entryOwnershipManagingContractIndex,
		                                                     locals.gameTemplate.entryPossessionManagingContractIndex);
		if (locals.possessedShares < static_cast<sint64>(locals.totalAmount))
		{
			output.returnCode = toReturnCode(EReturnCode::INSUFFICIENT_FUNDS);
			return;
		}

		if (locals.developer1Amount > 0)
		{
			locals.transferResult =
			    qpi.transferShareOwnershipAndPossession(locals.gameTemplate.entryAsset.assetName, locals.gameTemplate.entryAsset.issuer, SELF, SELF,
			                                            static_cast<sint64>(locals.developer1Amount), state.get().developer1);
			if (locals.transferResult < 0)
			{
				output.returnCode = toReturnCode(EReturnCode::INSUFFICIENT_FUNDS);
				return;
			}
			locals.gameTemplate.assetDeveloper1Accrued = 0;
		}
		if (locals.developer2Amount > 0)
		{
			locals.transferResult =
			    qpi.transferShareOwnershipAndPossession(locals.gameTemplate.entryAsset.assetName, locals.gameTemplate.entryAsset.issuer, SELF, SELF,
			                                            static_cast<sint64>(locals.developer2Amount), state.get().developer2);
			if (locals.transferResult < 0)
			{
				state.mut().templates.set(input.templateId, locals.gameTemplate);
				output.returnCode = toReturnCode(EReturnCode::INSUFFICIENT_FUNDS);
				return;
			}
			locals.gameTemplate.assetDeveloper2Accrued = 0;
		}
		locals.transferOutput.distributedAmount = 0;
		if (locals.dividendAmount > 0)
		{
			locals.transferInput.dividendAsset = locals.gameTemplate.entryAsset;
			locals.transferInput.shareholdersAsset.assetName = PULSEEDITOR_CONTRACT_ASSET_NAME;
			locals.transferInput.shareholdersAsset.issuer = id::zero();
			locals.transferInput.dividendAmount = static_cast<sint64>(locals.dividendAmount);
			locals.transferInput.shareholdersTotalShares = NUMBER_OF_COMPUTORS;
			CALL(TransferAssetDividendToShareholders, locals.transferInput, locals.transferOutput);
		}

		locals.gameTemplate.assetDividendAccrued = locals.gameTemplate.assetDividendAccrued - locals.transferOutput.distributedAmount;
		state.mut().templates.set(input.templateId, locals.gameTemplate);

		output.developer1Amount = locals.developer1Amount;
		output.developer2Amount = locals.developer2Amount;
		output.dividendAmount = locals.transferOutput.distributedAmount;
		output.dividendAccrued = locals.gameTemplate.assetDividendAccrued;
		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

	/**
	 * @brief Opens the next round from tick automation after the previous round settled.
	 * @param input Template id whose next round should be opened.
	 * @param output New round id and status code.
	 * @return `SUCCESS`, `INVALID_TEMPLATE`, `INVALID_STATE`, or `INSUFFICIENT_FUNDS`.
	 * @note This is not user-callable; reserve checks match publication readiness.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(StartNextRound)
	{
		if (!isTemplateIdValid(state, input.templateId))
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_TEMPLATE);
			return;
		}

		locals.gameTemplate = state.get().templates.get(input.templateId);
		locals.round = state.get().rounds.get(input.templateId);
		if (locals.gameTemplate.status != ETemplateStatus::PUBLISHED || locals.round.status != ERoundStatus::SETTLED)
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_STATE);
			return;
		}
		if (!hasRequiredBaseReserve(locals.gameTemplate))
		{
			output.returnCode = toReturnCode(EReturnCode::INSUFFICIENT_FUNDS);
			return;
		}
		if (locals.gameTemplate.bonusEnabled &&
		    ((locals.gameTemplate.rewardMode == ERewardMode::QUBIC && locals.gameTemplate.bonusReserve < requiredBonusReserve(locals.gameTemplate)) ||
		     (locals.gameTemplate.rewardMode == ERewardMode::ASSET &&
		      locals.gameTemplate.assetBonusReserve < requiredBonusReserve(locals.gameTemplate))))
		{
			output.returnCode = toReturnCode(EReturnCode::INSUFFICIENT_FUNDS);
			return;
		}
		if (locals.gameTemplate.roundEndTick > 0 && qpi.tick() > locals.gameTemplate.roundEndTick)
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_STATE);
			return;
		}

		locals.gameTemplate.currentRoundId = sadd(locals.gameTemplate.currentRoundId, 1U);
		setMemory(locals.round, 0);
		locals.round.roundId = locals.gameTemplate.currentRoundId;
		locals.round.startTick = locals.gameTemplate.roundStartTick;
		locals.round.endTick = locals.gameTemplate.roundEndTick;
		locals.round.status = ERoundStatus::SELLING;

		state.mut().templates.set(input.templateId, locals.gameTemplate);
		state.mut().rounds.set(input.templateId, locals.round);

		output.roundId = locals.round.roundId;
		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

	/**
	 * @brief Reads a full template snapshot.
	 * @param input Template id to query.
	 * @param output Template data and status code.
	 * @return `SUCCESS` or `INVALID_TEMPLATE`.
	 */
	PUBLIC_FUNCTION(GetTemplate)
	{
		if (!isTemplateIdValid(state, input.templateId))
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_TEMPLATE);
			return;
		}

		output.gameTemplate = state.get().templates.get(input.templateId);
		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

	/**
	 * @brief Reads the current round slot for a template.
	 * @param input Template id to query.
	 * @param output Round data and status code.
	 * @return `SUCCESS` or `INVALID_TEMPLATE`.
	 */
	PUBLIC_FUNCTION(GetRound)
	{
		if (!isTemplateIdValid(state, input.templateId))
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_TEMPLATE);
			return;
		}

		output.round = state.get().rounds.get(input.templateId);
		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

	/**
	 * @brief Checks whether a template has enough funding and a valid lifecycle state for publication or tick-started rounds.
	 * @param input Template id to inspect.
	 * @param output Funding requirements, current reserves, readiness flag, and first blocker.
	 * @return `SUCCESS` or `INVALID_TEMPLATE`.
	 * @note For drafts this evaluates `PublishTemplate`; for published settled templates this mirrors tick automation readiness.
	 */
	PUBLIC_FUNCTION_WITH_LOCALS(GetTemplateReadiness)
	{
		output.requiredPrizeReserve = 0;
		output.requiredBonusReserve = 0;
		output.prizeReserve = 0;
		output.bonusReserve = 0;
		output.assetPrizeReserve = 0;
		output.assetBonusReserve = 0;
		output.currentTick = qpi.tick();
		output.reasonCode = toReturnCode(EReturnCode::UNKNOWN_ERROR);
		output.isReady = false;

		if (!isTemplateIdValid(state, input.templateId))
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_TEMPLATE);
			return;
		}

		locals.gameTemplate = state.get().templates.get(input.templateId);
		locals.round = state.get().rounds.get(input.templateId);
		output.requiredPrizeReserve = requiredBasePrizeReserve(locals.gameTemplate);
		output.requiredBonusReserve = requiredBonusReserve(locals.gameTemplate);
		output.prizeReserve = locals.gameTemplate.prizeReserve;
		output.bonusReserve = locals.gameTemplate.bonusReserve;
		output.assetPrizeReserve = locals.gameTemplate.assetPrizeReserve;
		output.assetBonusReserve = locals.gameTemplate.assetBonusReserve;

		if (locals.gameTemplate.status != ETemplateStatus::DRAFT &&
		    (locals.gameTemplate.status != ETemplateStatus::PUBLISHED || locals.round.status != ERoundStatus::SETTLED))
		{
			output.reasonCode = toReturnCode(EReturnCode::INVALID_STATE);
			output.returnCode = toReturnCode(EReturnCode::SUCCESS);
			return;
		}
		if (!hasRequiredBaseReserve(locals.gameTemplate))
		{
			output.reasonCode = toReturnCode(EReturnCode::INSUFFICIENT_FUNDS);
			output.returnCode = toReturnCode(EReturnCode::SUCCESS);
			return;
		}
		if (locals.gameTemplate.bonusEnabled &&
		    ((locals.gameTemplate.rewardMode == ERewardMode::QUBIC && locals.gameTemplate.bonusReserve < requiredBonusReserve(locals.gameTemplate)) ||
		     (locals.gameTemplate.rewardMode == ERewardMode::ASSET &&
		      locals.gameTemplate.assetBonusReserve < requiredBonusReserve(locals.gameTemplate))))
		{
			output.reasonCode = toReturnCode(EReturnCode::INSUFFICIENT_FUNDS);
			output.returnCode = toReturnCode(EReturnCode::SUCCESS);
			return;
		}
		if (locals.gameTemplate.roundEndTick > 0 && qpi.tick() > locals.gameTemplate.roundEndTick)
		{
			output.reasonCode = toReturnCode(EReturnCode::INVALID_STATE);
			output.returnCode = toReturnCode(EReturnCode::SUCCESS);
			return;
		}

		output.reasonCode = toReturnCode(EReturnCode::SUCCESS);
		output.isReady = true;
		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

	/**
	 * @brief Reads a page of created templates for simple on-chain discovery.
	 * @param input Offset and page limit.
	 * @param output Template ids, status bytes, total count, and status code.
	 * @return `SUCCESS` or `INVALID_VALUE`.
	 */
	PUBLIC_FUNCTION_WITH_LOCALS(GetTemplates)
	{
		setMemory(output.templateIds, 0);
		setMemory(output.statuses, 0);
		output.totalTemplates = state.get().templateCount;
		output.returnedCount = 0;

		if (input.offset > state.get().templateCount)
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_VALUE);
			return;
		}

		locals.requestedLimit =
		    input.limit == 0 || input.limit > output.templateIds.capacity() ? static_cast<uint16>(output.templateIds.capacity()) : input.limit;
		locals.remaining = state.get().templateCount - input.offset;
		output.returnedCount = static_cast<uint16>(locals.remaining < locals.requestedLimit ? locals.remaining : locals.requestedLimit);

		for (locals.i = 0; locals.i < output.returnedCount; ++locals.i)
		{
			output.templateIds.set(locals.i, static_cast<uint16>(input.offset + locals.i));
			output.statuses.set(locals.i, static_cast<uint8>(state.get().templates.get(input.offset + locals.i).status));
		}

		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

	/**
	 * @brief Reads a ticket by global ticket index.
	 * @param input Ticket index returned by `BuyTicket`.
	 * @param output Ticket data and status code.
	 * @return `SUCCESS` or `INVALID_VALUE`.
	 */
	PUBLIC_FUNCTION(GetTicket)
	{
		if (input.ticketIndex >= state.get().ticketCount)
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_VALUE);
			return;
		}

		output.ticket = state.get().tickets.get(input.ticketIndex);
		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

	/**
	 * @brief Reads one chronological page of tickets bought by a player.
	 * @param input Player id, optional template/round filters, offset, and page size.
	 * @param output Ticket page, global ticket indexes, total matched count, and status code.
	 * @return `SUCCESS`, `INVALID_TEMPLATE`, or `INVALID_VALUE`.
	 * @note The function scans retained tickets because the MVP stores tickets in one global append-only array.
	 */
	PUBLIC_FUNCTION_WITH_LOCALS(GetPlayerTickets)
	{
		setMemory(output.tickets, 0);
		setMemory(output.ticketIndexes, 0);
		output.totalMatched = 0;
		output.returnedCount = 0;

		if (input.player == NULL_ID)
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_VALUE);
			return;
		}
		if (input.useTemplateFilter && !isTemplateIdValid(state, input.templateId))
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_TEMPLATE);
			return;
		}

		locals.requestedLimit =
		    input.limit == 0 || input.limit > output.tickets.capacity() ? static_cast<uint16>(output.tickets.capacity()) : input.limit;
		locals.matchedCount = 0;

		for (locals.i = 0; locals.i < state.get().ticketCount; ++locals.i)
		{
			locals.ticket = state.get().tickets.get(locals.i);
			if (locals.ticket.player != input.player)
			{
				continue;
			}
			if (input.useTemplateFilter && locals.ticket.templateId != input.templateId)
			{
				continue;
			}
			if (input.useRoundFilter && locals.ticket.roundId != input.roundId)
			{
				continue;
			}

			if (locals.matchedCount >= input.offset && output.returnedCount < locals.requestedLimit)
			{
				output.tickets.set(output.returnedCount, locals.ticket);
				output.ticketIndexes.set(output.returnedCount, locals.i);
				++output.returnedCount;
			}
			locals.matchedCount = sadd(locals.matchedCount, 1ULL);
		}

		output.totalMatched = locals.matchedCount;
		if (input.offset > output.totalMatched)
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_VALUE);
			return;
		}

		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

	/**
	 * @brief Reads one chronological page from the winner-history ring buffer.
	 * @param input Offset and requested page size.
	 * @param output Winner page, counters, and status code.
	 * @return `SUCCESS` or `INVALID_VALUE` when `offset` is outside the retained history window.
	 * @note The retained window is capped by `PULSEEDITOR_MAX_WINNERS`; older entries are overwritten.
	 */
	PUBLIC_FUNCTION_WITH_LOCALS(GetWinners)
	{
		setMemory(output.winners, 0);
		output.winnerCounter = state.get().winnerCounter;
		output.totalStored = state.get().winnerCounter < state.get().winners.capacity() ? state.get().winnerCounter : state.get().winners.capacity();
		output.returnedCount = 0;

		if (input.offset >= output.totalStored)
		{
			output.returnCode = input.offset == 0 ? toReturnCode(EReturnCode::SUCCESS) : toReturnCode(EReturnCode::INVALID_VALUE);
			return;
		}

		locals.requestedLimit =
		    input.limit == 0 || input.limit > output.winners.capacity() ? static_cast<uint16>(output.winners.capacity()) : input.limit;
		locals.remaining = output.totalStored - input.offset;
		output.returnedCount = static_cast<uint16>(locals.remaining < locals.requestedLimit ? locals.remaining : locals.requestedLimit);
		locals.oldestCounter = state.get().winnerCounter - output.totalStored;

		for (locals.i = 0; locals.i < output.returnedCount; ++locals.i)
		{
			locals.sourceCounter = locals.oldestCounter + input.offset + locals.i;
			locals.sourceIndex = mod(locals.sourceCounter, state.get().winners.capacity());
			output.winners.set(locals.i, state.get().winners.get(locals.sourceIndex));
		}

		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

	/**
	 * @brief Reads current platform fee recipients and pending accrued balances.
	 * @param input Empty input.
	 * @param output Platform owner, developer recipient ids, accrued balances, fee percent, creator limit, and status code.
	 * @return Always `SUCCESS`.
	 */
	PUBLIC_FUNCTION(GetPlatformAccounting)
	{
		output.platformOwner = state.get().platformOwner;
		output.developer1 = state.get().developer1;
		output.developer2 = state.get().developer2;
		output.developer1Accrued = state.get().developer1Accrued;
		output.developer2Accrued = state.get().developer2Accrued;
		output.dividendAccrued = state.get().dividendAccrued;
		output.platformFeePercent = state.get().platformFeePercent;
		output.maxCreatorFeePercent = state.get().maxCreatorFeePercent;
		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

	/**
	 * @brief Validates a code against length, digit range, and duplicate rules.
	 * @param input Digits and validation rules.
	 * @param output Validation result and status code.
	 * @return `SUCCESS` when valid; otherwise `INVALID_DIGITS`.
	 * @note Only the first `codeLength` digit slots are checked.
	 */
	PUBLIC_FUNCTION_WITH_LOCALS(ValidateDigits)
	{
		output.isValid = false;
		output.returnCode = toReturnCode(EReturnCode::INVALID_DIGITS);
		setMemory(locals.seen, 0);

		if (input.codeLength == 0 || input.codeLength > PULSEEDITOR_MAX_CODE_LENGTH || input.maxDigit > PULSEEDITOR_MAX_DIGIT)
		{
			return;
		}
		if (!input.allowRepeatedDigits && input.codeLength > static_cast<uint8>(input.maxDigit + 1))
		{
			return;
		}

		for (locals.i = 0; locals.i < input.codeLength; ++locals.i)
		{
			locals.digit = input.digits.get(locals.i);
			if (locals.digit > input.maxDigit)
			{
				return;
			}
			if (!input.allowRepeatedDigits)
			{
				locals.count = locals.seen.get(locals.digit);
				if (locals.count > 0)
				{
					return;
				}
				locals.seen.set(locals.digit, 1);
			}
		}

		output.isValid = true;
		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

	/**
	 * @brief Releases asset share management rights from this contract to another managing contract.
	 * @param input Asset, share count, and destination managing contract index.
	 * @param output Number of shares released, or zero on failure.
	 * @return No separate return code; failure is represented by zero transferred shares.
	 * @note Attached invocation reward is used as the maximum fee and any unused amount is refunded.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(TransferShareManagementRights)
	{
		locals.reward = qpi.invocationReward();
		locals.refundAmount = locals.reward;
		locals.success = false;
		output.transferredNumberOfShares = 0;

		if (input.numberOfShares > 0 && qpi.numberOfPossessedShares(input.asset.assetName, input.asset.issuer, qpi.invocator(), qpi.invocator(),
		                                                            SELF_INDEX, SELF_INDEX) >= input.numberOfShares)
		{
			locals.result = qpi.releaseShares(input.asset, qpi.invocator(), qpi.invocator(), input.numberOfShares, input.newManagingContractIndex,
			                                  input.newManagingContractIndex, locals.reward);
			if (locals.result != INVALID_AMOUNT && locals.result >= 0)
			{
				locals.success = true;
				locals.refundAmount = locals.reward - locals.result;
			}
		}

		if (locals.success)
		{
			output.transferredNumberOfShares = input.numberOfShares;
		}

		if (locals.refundAmount > 0)
		{
			qpi.transfer(qpi.invocator(), locals.refundAmount);
		}
	}

	/**
	 * @brief Settles the just-purchased ticket for instant-play templates.
	 * @param input Template id and global ticket index created by the purchase call.
	 * @param output `SUCCESS` or an internal settlement error.
	 * @note This private procedure never refunds invocation reward; the caller already accepted the ticket payment.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(SettleInstantTicket)
	{
		output.returnCode = toReturnCode(EReturnCode::UNKNOWN_ERROR);
		if (!isTemplateIdValid(state, input.templateId) || input.ticketIndex >= state.get().ticketCount)
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_VALUE);
			return;
		}

		locals.gameTemplate = state.get().templates.get(input.templateId);
		locals.round = state.get().rounds.get(input.templateId);
		locals.ticket = state.get().tickets.get(input.ticketIndex);
		if (!locals.gameTemplate.instantSettlement || locals.ticket.status != ETicketStatus::ACTIVE || locals.ticket.templateId != input.templateId ||
		    locals.ticket.roundId != locals.round.roundId)
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_STATE);
			return;
		}

		locals.round.status = ERoundStatus::CLOSED;
		locals.randomData.prevSpectrumDigest = qpi.getPrevSpectrumDigest();
		locals.randomData.templateId = input.templateId;
		locals.randomData.roundId = locals.round.roundId;
		locals.randomData.ticketCount = locals.round.ticketCount;
		locals.seed = qpi.K12(locals.randomData).u64._0;
		locals.generateInput.seed = locals.seed;
		locals.generateInput.codeLength = locals.gameTemplate.codeLength;
		locals.generateInput.maxDigit = locals.gameTemplate.maxDigit;
		locals.generateInput.allowRepeatedDigits = locals.gameTemplate.allowRepeatedDigits;
		CALL(GenerateWinningDigits, locals.generateInput, locals.generateOutput);

		locals.round.winningDigits = locals.generateOutput.digits;
		locals.countInput.playerDigits = locals.ticket.digits;
		locals.countInput.winningDigits = locals.round.winningDigits;
		locals.countInput.codeLength = locals.gameTemplate.codeLength;
		CALL(CountMatches, locals.countInput, locals.countOutput);

		locals.ticket.exact = locals.countOutput.exact;
		locals.ticket.misplaced = locals.countOutput.misplaced;
		locals.payout = locals.gameTemplate.payoutMatrix.get(locals.countOutput.payoutMatrixIndex);
		locals.bonusPayout = 0;
		locals.bonusQualified = false;

		if (locals.payout > 0)
		{
			locals.round.winnerCount = 1;
			if (locals.gameTemplate.bonusEnabled && locals.gameTemplate.bonusMultiplierBps > PULSEEDITOR_BONUS_MULTIPLIER_SCALE)
			{
				for (locals.bonusAssetIndex = 0; locals.bonusAssetIndex < locals.gameTemplate.bonusAssetCount; ++locals.bonusAssetIndex)
				{
					locals.bonusShares = qpi.numberOfPossessedShares(
					    locals.gameTemplate.bonusAssets.get(locals.bonusAssetIndex).assetName,
					    locals.gameTemplate.bonusAssets.get(locals.bonusAssetIndex).issuer, locals.ticket.player, locals.ticket.player,
					    locals.gameTemplate.bonusOwnershipManagingContractIndex, locals.gameTemplate.bonusPossessionManagingContractIndex);
					if (locals.bonusShares > 0)
					{
						locals.bonusQualified = true;
						break;
					}
				}
				locals.bonusPayout = bonusExtraPayout(locals.payout, locals.gameTemplate.bonusMultiplierBps);
				if (!locals.bonusQualified || !((locals.gameTemplate.rewardMode == ERewardMode::QUBIC &&
				                                 locals.gameTemplate.bonusReserve >= locals.bonusPayout) ||
				                                (locals.gameTemplate.rewardMode == ERewardMode::ASSET &&
				                                 locals.gameTemplate.assetBonusReserve >= locals.bonusPayout)))
				{
					locals.bonusPayout = 0;
				}
			}

			locals.totalPayout = sadd(locals.payout, locals.bonusPayout);
			locals.ticket.payout = locals.gameTemplate.rewardMode == ERewardMode::QUBIC ? locals.totalPayout : 0;
			locals.ticket.bonusPayout = locals.gameTemplate.rewardMode == ERewardMode::QUBIC ? locals.bonusPayout : 0;
			locals.ticket.assetPayout = locals.gameTemplate.rewardMode == ERewardMode::ASSET ? locals.totalPayout : 0;
			locals.ticket.assetBonusPayout = locals.gameTemplate.rewardMode == ERewardMode::ASSET ? locals.bonusPayout : 0;

			if (locals.gameTemplate.rewardMode == ERewardMode::QUBIC && locals.gameTemplate.prizeReserve >= locals.payout)
			{
				qpi.transfer(locals.ticket.player, locals.totalPayout);
				locals.gameTemplate.prizeReserve = locals.gameTemplate.prizeReserve - locals.payout;
				locals.gameTemplate.bonusReserve = locals.gameTemplate.bonusReserve - locals.bonusPayout;
				locals.gameTemplate.totalPaid = sadd(locals.gameTemplate.totalPaid, locals.totalPayout);
				locals.gameTemplate.totalBonusPaid = sadd(locals.gameTemplate.totalBonusPaid, locals.bonusPayout);
				locals.round.paid = locals.totalPayout;
				locals.ticket.status = ETicketStatus::PAID;
			}
			else if (locals.gameTemplate.rewardMode == ERewardMode::ASSET && locals.gameTemplate.assetPrizeReserve >= locals.payout &&
			         locals.totalPayout <= static_cast<uint64>(INT64_MAX))
			{
				locals.transferResult =
				    qpi.transferShareOwnershipAndPossession(locals.gameTemplate.rewardAsset.assetName, locals.gameTemplate.rewardAsset.issuer, SELF,
				                                            SELF, static_cast<sint64>(locals.totalPayout), locals.ticket.player);
				if (locals.transferResult >= 0)
				{
					locals.gameTemplate.assetPrizeReserve = locals.gameTemplate.assetPrizeReserve - locals.payout;
					locals.gameTemplate.assetBonusReserve = locals.gameTemplate.assetBonusReserve - locals.bonusPayout;
					locals.gameTemplate.totalAssetPaid = sadd(locals.gameTemplate.totalAssetPaid, locals.totalPayout);
					locals.gameTemplate.totalBonusPaid = sadd(locals.gameTemplate.totalBonusPaid, locals.bonusPayout);
					locals.round.paid = locals.totalPayout;
					locals.ticket.status = ETicketStatus::PAID;
				}
			}
			if (locals.ticket.status != ETicketStatus::PAID)
			{
				locals.round.unpaidWinnerCount = 1;
				locals.ticket.status = ETicketStatus::UNPAID;
			}
			else
			{
				locals.winnerInfo.epoch = qpi.epoch();
				locals.winnerInfo.tick = qpi.tick();
				locals.winnerInfo.player = locals.ticket.player;
				locals.winnerInfo.payout = locals.ticket.payout;
				locals.winnerInfo.bonusPayout = locals.ticket.bonusPayout;
				locals.winnerInfo.assetPayout = locals.ticket.assetPayout;
				locals.winnerInfo.assetBonusPayout = locals.ticket.assetBonusPayout;
				locals.winnerInfo.roundId = locals.round.roundId;
				locals.winnerInfo.templateId = input.templateId;
				locals.winnerInfo.exact = locals.ticket.exact;
				locals.winnerInfo.misplaced = locals.ticket.misplaced;
				locals.winnerIndex = mod(state.get().winnerCounter, state.get().winners.capacity());
				state.mut().winners.set(locals.winnerIndex, locals.winnerInfo);
				state.mut().winnerCounter = sadd(state.get().winnerCounter, 1ULL);
			}
		}
		else
		{
			locals.ticket.payout = 0;
			locals.ticket.bonusPayout = 0;
			locals.ticket.assetPayout = 0;
			locals.ticket.assetBonusPayout = 0;
			locals.ticket.status = ETicketStatus::PAID;
		}

		locals.round.settledTick = qpi.tick();
		locals.round.status = ERoundStatus::SETTLED;
		locals.gameTemplate.lastDrawEpoch = qpi.epoch();
		state.mut().tickets.set(input.ticketIndex, locals.ticket);

		if (locals.gameTemplate.status == ETemplateStatus::STOP_REQUESTED)
		{
			locals.gameTemplate.status = ETemplateStatus::STOPPED;
		}
		else if (hasRequiredBaseReserve(locals.gameTemplate) && (!locals.gameTemplate.bonusEnabled ||
		                                                         (locals.gameTemplate.rewardMode == ERewardMode::QUBIC &&
		                                                          locals.gameTemplate.bonusReserve >= requiredBonusReserve(locals.gameTemplate)) ||
		                                                         (locals.gameTemplate.rewardMode == ERewardMode::ASSET &&
		                                                          locals.gameTemplate.assetBonusReserve >= requiredBonusReserve(locals.gameTemplate))))
		{
			locals.gameTemplate.currentRoundId = sadd(locals.gameTemplate.currentRoundId, 1U);
			setMemory(locals.round, 0);
			locals.round.roundId = locals.gameTemplate.currentRoundId;
			locals.round.startTick = locals.gameTemplate.roundStartTick;
			locals.round.endTick = locals.gameTemplate.roundEndTick;
			locals.round.status = ERoundStatus::SELLING;
		}

		state.mut().templates.set(input.templateId, locals.gameTemplate);
		state.mut().rounds.set(input.templateId, locals.round);
		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

	/**
	 * @brief Refunds template-local balances to the owner and clears an idle template slot.
	 * @param input Template id selected by lifecycle automation.
	 * @param output Refunded Qubic/asset amounts and status code.
	 * @return `SUCCESS`, `INVALID_TEMPLATE`, or `INSUFFICIENT_FUNDS`.
	 * @note Qubic platform accruals are global and are not part of template-local deletion refunds.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(DeleteIdleTemplate)
	{
		output.qubicRefund = 0;
		output.assetRefund = 0;
		if (!isTemplateIdValid(state, input.templateId))
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_TEMPLATE);
			return;
		}

		locals.gameTemplate = state.get().templates.get(input.templateId);
		locals.round = state.get().rounds.get(input.templateId);
		locals.qubicRefund =
		    sadd(sadd(locals.gameTemplate.prizeReserve, locals.gameTemplate.bonusReserve),
		         sadd(locals.gameTemplate.creatorRevenue, locals.gameTemplate.burnAccrued));
		locals.assetRefund =
		    sadd(sadd(locals.gameTemplate.assetPrizeReserve, locals.gameTemplate.assetBonusReserve),
		         sadd(sadd(locals.gameTemplate.assetCreatorRevenue, locals.gameTemplate.assetBurnAccrued),
		              sadd(locals.gameTemplate.assetDeveloper1Accrued,
		                   sadd(locals.gameTemplate.assetDeveloper2Accrued, locals.gameTemplate.assetDividendAccrued))));

		if (locals.assetRefund > 0)
		{
			if (locals.gameTemplate.entryMode != EEntryMode::ASSET || locals.assetRefund > static_cast<uint64>(INT64_MAX))
			{
				output.returnCode = toReturnCode(EReturnCode::INSUFFICIENT_FUNDS);
				return;
			}
			locals.transferResult =
			    qpi.transferShareOwnershipAndPossession(locals.gameTemplate.entryAsset.assetName, locals.gameTemplate.entryAsset.issuer, SELF, SELF,
			                                            static_cast<sint64>(locals.assetRefund), locals.gameTemplate.owner);
			if (locals.transferResult < 0)
			{
				output.returnCode = toReturnCode(EReturnCode::INSUFFICIENT_FUNDS);
				return;
			}
		}
		if (locals.qubicRefund > 0)
		{
			qpi.transfer(locals.gameTemplate.owner, locals.qubicRefund);
		}

		setMemory(locals.gameTemplate, 0);
		setMemory(locals.round, 0);
		state.mut().templates.set(input.templateId, locals.gameTemplate);
		state.mut().rounds.set(input.templateId, locals.round);

		output.qubicRefund = locals.qubicRefund;
		output.assetRefund = locals.assetRefund;
		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

	/**
	 * @brief Advances lifecycle automation for a bounded page of templates.
	 * @param input Empty input; the procedure reads automation cursor and template state.
	 * @param output Number of inspected template slots and attempted lifecycle actions.
	 * @note Only published or stop-requested templates can be settled, and only published settled templates can start another round.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(ProcessLifecycleAutomation)
	{
		output.inspectedTemplates = 0;
		output.lifecycleActions = 0;
		if (state.get().templateCount == 0)
		{
			return;
		}

		locals.templatesToInspect = state.get().templateCount < PULSEEDITOR_AUTOMATION_TEMPLATES_PER_TICK
		                                ? state.get().templateCount
		                                : PULSEEDITOR_AUTOMATION_TEMPLATES_PER_TICK;
		for (locals.i = 0; locals.i < locals.templatesToInspect; ++locals.i)
		{
			locals.templateIndex = mod(sadd(static_cast<uint64>(state.get().automationCursor), locals.i), state.get().templateCount);
			locals.gameTemplate = state.get().templates.get(locals.templateIndex);
			locals.round = state.get().rounds.get(locals.templateIndex);
			++output.inspectedTemplates;

			if (locals.gameTemplate.status == ETemplateStatus::EMPTY)
			{
				continue;
			}

			if (static_cast<uint64>(qpi.epoch()) >=
			    sadd(static_cast<uint64>(locals.gameTemplate.lastDrawEpoch), static_cast<uint64>(PULSEEDITOR_TEMPLATE_IDLE_EPOCH_LIMIT)))
			{
				locals.deleteInput.templateId = static_cast<uint16>(locals.templateIndex);
				CALL(DeleteIdleTemplate, locals.deleteInput, locals.deleteOutput);
				++output.lifecycleActions;
				continue;
			}

			if (locals.gameTemplate.status != ETemplateStatus::PUBLISHED && locals.gameTemplate.status != ETemplateStatus::STOP_REQUESTED)
			{
				continue;
			}

			if (locals.round.status == ERoundStatus::CLOSED ||
			    (locals.round.status == ERoundStatus::SELLING &&
			     (locals.round.ticketCount >= locals.gameTemplate.ticketLimit || (locals.round.endTick > 0 && qpi.tick() > locals.round.endTick))))
			{
				locals.settleInput.templateId = static_cast<uint16>(locals.templateIndex);
				CALL(SettleRound, locals.settleInput, locals.settleOutput);
				++output.lifecycleActions;
			}

			locals.gameTemplate = state.get().templates.get(locals.templateIndex);
			locals.round = state.get().rounds.get(locals.templateIndex);
			if (locals.gameTemplate.status == ETemplateStatus::PUBLISHED && locals.round.status == ERoundStatus::SETTLED)
			{
				locals.startInput.templateId = static_cast<uint16>(locals.templateIndex);
				CALL(StartNextRound, locals.startInput, locals.startOutput);
				++output.lifecycleActions;
			}
		}

		state.mut().automationCursor =
		    static_cast<uint16>(mod(sadd(static_cast<uint64>(state.get().automationCursor), locals.templatesToInspect), state.get().templateCount));
	}

	/**
	 * @brief Transfers managed asset dividends to contract-share holders.
	 * @param input Dividend asset, contract-share asset, dividend amount, and total share count.
	 * @param output Number of asset shares scheduled for distribution.
	 * @note Remainder below one share per contract share is retained by the caller.
	 */
	PRIVATE_PROCEDURE_WITH_LOCALS(TransferAssetDividendToShareholders)
	{
		output.distributedAmount = 0;
		if (input.dividendAmount <= 0 || input.shareholdersTotalShares <= 0 || input.dividendAsset.assetName == 0 ||
		    input.shareholdersAsset.assetName == 0)
		{
			return;
		}

		locals.dividendPerShare = div<sint64>(input.dividendAmount, input.shareholdersTotalShares);
		if (locals.dividendPerShare <= 0)
		{
			return;
		}

		locals.shareholdersIter.begin(input.shareholdersAsset);
		while (!locals.shareholdersIter.reachedEnd())
		{
			locals.holderShares = locals.shareholdersIter.numberOfPossessedShares();
			if (locals.holderShares > 0)
			{
				locals.holderDividend = smul(locals.holderShares, locals.dividendPerShare);
				locals.transferResult = qpi.transferShareOwnershipAndPossession(input.dividendAsset.assetName, input.dividendAsset.issuer, SELF, SELF,
				                                                                locals.holderDividend, locals.shareholdersIter.possessor());
				if (locals.transferResult >= 0)
				{
					output.distributedAmount = sadd(output.distributedAmount, static_cast<uint64>(locals.holderDividend));
				}
			}
			locals.shareholdersIter.next();
		}
	}

	/**
	 * @brief Counts exact and misplaced matches between player and winning digits.
	 * @param input Player code, winning code, and active code length.
	 * @param output Exact count, misplaced count, and payout matrix index.
	 * @return Internal output struct filled deterministically.
	 * @note Exact matches are removed before misplaced counts to prevent double counting repeated digits.
	 */
	PRIVATE_FUNCTION_WITH_LOCALS(CountMatches)
	{
		output.exact = 0;
		output.misplaced = 0;
		output.payoutMatrixIndex = 0;
		setMemory(locals.playerCounts, 0);
		setMemory(locals.winningCounts, 0);

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

		for (locals.i = 0; locals.i <= PULSEEDITOR_MAX_DIGIT; ++locals.i)
		{
			locals.playerCount = locals.playerCounts.get(locals.i);
			locals.winningCount = locals.winningCounts.get(locals.i);
			output.misplaced += (locals.playerCount < locals.winningCount) ? locals.playerCount : locals.winningCount;
		}

		output.payoutMatrixIndex = static_cast<uint16>(
		    sadd(smul(static_cast<uint64>(output.exact), static_cast<uint64>(PULSEEDITOR_MATRIX_SIDE)), static_cast<uint64>(output.misplaced)));
	}

	/**
	 * @brief Generates winning digits from a deterministic seed.
	 * @param input Seed, code length, digit range, and duplicate policy.
	 * @param output Generated winning code.
	 * @return Internal output struct with QPI-aligned digit storage.
	 * @note If unique generation repeatedly collides, the function falls back to the first unused digit.
	 */
	PRIVATE_FUNCTION_WITH_LOCALS(GenerateWinningDigits)
	{
		setMemory(locals.used, 0);
		setMemory(output.digits, 0);

		for (locals.index = 0; locals.index < input.codeLength; ++locals.index)
		{
			deriveOne(input.seed, locals.index, locals.tempValue);
			locals.candidate = static_cast<uint8>(mod(locals.tempValue, sadd(static_cast<uint64>(input.maxDigit), 1ULL)));
			locals.attempts = 0;

			while (!input.allowRepeatedDigits && locals.used.get(locals.candidate) > 0 && locals.attempts < PULSEEDITOR_RANDOM_RETRY_LIMIT)
			{
				++locals.attempts;
				mix64(locals.tempValue, locals.tempValue);
				locals.candidate = static_cast<uint8>(mod(locals.tempValue, sadd(static_cast<uint64>(input.maxDigit), 1ULL)));
			}
			if (!input.allowRepeatedDigits && locals.used.get(locals.candidate) > 0)
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

	/**
	 * @brief Checks whether a template id points to an initialized template.
	 * @param state Contract state.
	 * @param templateId Template index to validate.
	 * @return True when the id is inside the created range and the template slot is not empty.
	 */
	static bool isTemplateIdValid(const QPI::ContractState<StateData, CONTRACT_INDEX>& state, const uint16 templateId)
	{
		return templateId < state.get().templateCount && state.get().templates.get(templateId).status != ETemplateStatus::EMPTY;
	}

	/**
	 * @brief Validates the optional scheduled selling window.
	 * @param startTick First tick that accepts purchases; zero means no lower bound.
	 * @param endTick Last tick that accepts purchases; zero disables time-based auto-close.
	 * @return True when the optional bounds are monotonic.
	 */
	static bool isScheduleConfigValid(const uint32 startTick, const uint32 endTick) { return endTick == 0 || startTick <= endTick; }

	/**
	 * @brief Encodes QPI calendar fields into a compact bootstrap-time guard stamp.
	 * @param year Current QPI year value.
	 * @param month Current QPI month value.
	 * @param day Current QPI day value.
	 * @param result Encoded sortable stamp.
	 */
	static void makeDateStamp(const uint8 year, const uint8 month, const uint8 day, uint32& result)
	{
		result = static_cast<uint32>(year << 9 | month << 5 | day);
	}

	/**
	 * @brief Validates multiplier-bonus settings before storing a template configuration.
	 * @param bonusEnabled Whether bonus checks are enabled.
	 * @param bonusMultiplierBps Multiplier in basis points; must be greater than `1.0x` when enabled.
	 * @param bonusAssetCount Number of qualifying assets configured for the template.
	 * @param ownershipManagingContractIndex Ownership managing contract used for possession lookup.
	 * @param possessionManagingContractIndex Possession managing contract used for possession lookup.
	 * @return True when disabled or when the enabled bonus has at least one asset and a valid multiplier.
	 */
	static bool isBonusConfigValid(const bit bonusEnabled, const uint32 bonusMultiplierBps, const uint16 bonusAssetCount,
	                               const uint16 ownershipManagingContractIndex, const uint16 possessionManagingContractIndex)
	{
		return !bonusEnabled ||
		       (bonusMultiplierBps > PULSEEDITOR_BONUS_MULTIPLIER_SCALE &&
		        bonusMultiplierBps <= PULSEEDITOR_MAX_BONUS_MULTIPLIER_BPS && bonusAssetCount > 0 &&
		        bonusAssetCount <= PULSEEDITOR_MAX_BONUS_ASSETS && ownershipManagingContractIndex > 0 &&
		        possessionManagingContractIndex > 0);
	}

	/**
	 * @brief Validates reward-currency settings before storing a template configuration.
	 * @param rewardMode Qubic or asset-share payout mode.
	 * @param rewardAsset Asset paid to winners in asset mode.
	 * @param ownershipManagingContractIndex Ownership managing contract used for reward asset reserve/payout.
	 * @param possessionManagingContractIndex Possession managing contract used for reward asset reserve/payout.
	 * @return True when Qubic mode has no extra requirements or asset mode has a valid managed asset identity.
	 */
	static bool isRewardConfigValid(const ERewardMode rewardMode, const Asset& rewardAsset, const uint16 ownershipManagingContractIndex,
	                                const uint16 possessionManagingContractIndex)
	{
		return rewardMode == ERewardMode::QUBIC || (rewardMode == ERewardMode::ASSET && rewardAsset.assetName != 0 && rewardAsset.issuer != NULL_ID &&
		                                            ownershipManagingContractIndex > 0 && possessionManagingContractIndex > 0);
	}

	/**
	 * @brief Validates the ticket-payment asset configuration for a template.
	 * @param entryMode Ticket payment mode.
	 * @param entryAsset Asset collected from players when `entryMode == ASSET`.
	 * @param rewardMode Reward payout mode selected by the template.
	 * @param rewardAsset Asset paid to winners when `rewardMode == ASSET`.
	 * @param ownershipManagingContractIndex Ownership managing contract used to collect asset ticket payments.
	 * @param possessionManagingContractIndex Possession managing contract used to collect asset ticket payments.
	 * @return True when entry and reward currencies are both Qubic or the same managed asset.
	 * @note Asset-entry templates add collected shares directly to the asset prize reserve, so entry and reward assets must match.
	 */
	static bool isEntryConfigValid(const EEntryMode entryMode, const Asset& entryAsset, const ERewardMode rewardMode, const Asset& rewardAsset,
	                               const uint16 ownershipManagingContractIndex, const uint16 possessionManagingContractIndex)
	{
		return (entryMode == EEntryMode::QUBIC && rewardMode == ERewardMode::QUBIC) ||
		       (entryMode == EEntryMode::ASSET && rewardMode == ERewardMode::ASSET && entryAsset.assetName != 0 && entryAsset.issuer != NULL_ID &&
		        entryAsset.assetName == rewardAsset.assetName && entryAsset.issuer == rewardAsset.issuer && ownershipManagingContractIndex > 0 &&
		        possessionManagingContractIndex > 0);
	}

	/**
	 * @brief Computes the base reserve required before opening a round.
	 * @param gameTemplate Template whose funding requirement is evaluated.
	 * @return Minimum deposited reserve required before a round can start.
	 * @note Asset-entry templates collect and split one ticket before instant settlement, so the prize portion can cover part of the base payout.
	 */
	static uint64 requiredBasePrizeReserve(const GameTemplate& gameTemplate)
	{
		return gameTemplate.entryMode == EEntryMode::ASSET && gameTemplate.rewardMode == ERewardMode::ASSET
		           ? (gameTemplate.maxSinglePayout > assetEntryPrizeContribution(gameTemplate)
		                  ? gameTemplate.maxSinglePayout - assetEntryPrizeContribution(gameTemplate)
		                  : 0)
		           : gameTemplate.maxSinglePayout;
	}

	/**
	 * @brief Computes the maximum bonus reserve required by the configured multiplier.
	 * @param gameTemplate Template whose maximum base payout and multiplier are evaluated.
	 * @return Extra reserve needed to pay one maximum base win with the multiplier applied.
	 */
	static uint64 requiredBonusReserve(const GameTemplate& gameTemplate)
	{
		return gameTemplate.bonusEnabled ? bonusExtraPayout(gameTemplate.maxSinglePayout, gameTemplate.bonusMultiplierBps) : 0;
	}

	/**
	 * @brief Computes the extra payout added by a multiplier.
	 * @param basePayout Base payout before the bonus.
	 * @param bonusMultiplierBps Multiplier in basis points.
	 * @return Extra amount above `basePayout`; for `12000` and `300`, returns `60`.
	 */
	static uint64 bonusExtraPayout(const uint64 basePayout, const uint32 bonusMultiplierBps)
	{
		return bonusMultiplierBps > PULSEEDITOR_BONUS_MULTIPLIER_SCALE
		           ? div<uint64>(smul(basePayout, static_cast<uint64>(bonusMultiplierBps)), static_cast<uint64>(PULSEEDITOR_BONUS_MULTIPLIER_SCALE)) -
		                 basePayout
		           : 0;
	}

	/**
	 * @brief Computes the prize-reserve share produced by one asset ticket after all asset-entry fees.
	 * @param gameTemplate Template whose asset ticket economics are evaluated.
	 * @return Asset shares added to `assetPrizeReserve` by one accepted ticket.
	 */
	static uint64 assetEntryPrizeContribution(const GameTemplate& gameTemplate)
	{
		return (gameTemplate.ticketPrice -
		        div<uint64>(smul(gameTemplate.ticketPrice, static_cast<uint64>(PULSEEDITOR_PLATFORM_FEE_PERCENT)), 100ULL)) -
		       div<uint64>(smul((gameTemplate.ticketPrice -
		                         div<uint64>(smul(gameTemplate.ticketPrice, static_cast<uint64>(PULSEEDITOR_PLATFORM_FEE_PERCENT)), 100ULL)),
		                        static_cast<uint64>(gameTemplate.creatorFeePercent)),
		                   100ULL) -
		       div<uint64>(smul((gameTemplate.ticketPrice -
		                         div<uint64>(smul(gameTemplate.ticketPrice, static_cast<uint64>(PULSEEDITOR_PLATFORM_FEE_PERCENT)), 100ULL)),
		                        static_cast<uint64>(gameTemplate.burnPercent)),
		                   100ULL);
	}

	/**
	 * @brief Checks whether the selected reward reserve can safely open the next round.
	 * @param gameTemplate Template whose reserve balance is evaluated.
	 * @return True when the base reserve satisfies `requiredBasePrizeReserve`.
	 */
	static bool hasRequiredBaseReserve(const GameTemplate& gameTemplate)
	{
		return (gameTemplate.rewardMode == ERewardMode::QUBIC && gameTemplate.prizeReserve >= requiredBasePrizeReserve(gameTemplate)) ||
		       (gameTemplate.rewardMode == ERewardMode::ASSET && gameTemplate.assetPrizeReserve >= requiredBasePrizeReserve(gameTemplate));
	}

	/**
	 * @brief Validates template configuration bounds before template creation.
	 * @param codeLength Requested code length.
	 * @param maxDigit Requested maximum digit.
	 * @param ticketPrice Required price per ticket.
	 * @param ticketLimit Per-round ticket cap.
	 * @param playerTicketLimit Per-player ticket cap within a round.
	 * @param creatorFeePercent Creator share of non-platform revenue.
	 * @param burnPercent Burn share of non-platform revenue.
	 * @param maxCreatorFeePercent Current platform creator-fee limit.
	 * @return True when all values fit MVP bounds and economic percentages are safe.
	 */
	static bool isTemplateConfigValid(const uint8 codeLength, const uint8 maxDigit, const uint64 ticketPrice, const uint16 ticketLimit,
	                                  const uint16 playerTicketLimit, const uint8 creatorFeePercent, const uint8 burnPercent,
	                                  const uint8 maxCreatorFeePercent)
	{
		return codeLength > 0 && codeLength <= PULSEEDITOR_MAX_CODE_LENGTH && maxDigit <= PULSEEDITOR_MAX_DIGIT && ticketPrice > 0 &&
		       ticketLimit > 0 && ticketLimit <= PULSEEDITOR_MAX_TICKETS && playerTicketLimit > 0 && playerTicketLimit <= ticketLimit &&
		       creatorFeePercent <= maxCreatorFeePercent && burnPercent <= PULSEEDITOR_MAX_BURN_PERCENT &&
		       sadd(static_cast<uint64>(creatorFeePercent), static_cast<uint64>(burnPercent)) <= 100ULL;
	}

	/**
	 * @brief Derives one deterministic pseudo-random value from a base seed and index.
	 * @param r Base seed.
	 * @param idx Derivation index.
	 * @param outValue Mixed output value.
	 */
	static void deriveOne(const uint64& r, const uint64& idx, uint64& outValue) { mix64(r + 0x9e3779b97f4a7c15ULL * (idx + 1), outValue); }

	/**
	 * @brief Applies SplitMix64-style mixing to a 64-bit value.
	 * @param x Input value.
	 * @param outValue Mixed output value.
	 * @note Raw multiplication is intentional here because wraparound is part of the mixing algorithm.
	 */
	static void mix64(const uint64& x, uint64& outValue)
	{
		outValue = x;
		outValue ^= outValue >> 30;
		outValue *= 0xbf58476d1ce4e5b9ULL;
		outValue ^= outValue >> 27;
		outValue *= 0x94d049bb133111ebULL;
		outValue ^= outValue >> 31;
	}
};
