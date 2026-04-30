/**
 * @file PulseEditor.h
 * @brief MVP constructor for on-chain code-guessing games.
 *
 * The MVP intentionally supports only Qubic entry / Qubic reward games with
 * fixed payouts. Each template has one active round at a time.
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
constexpr uint16 PULSEEDITOR_WINNERS_PAGE_SIZE = 1024;
static_assert((PULSEEDITOR_WINNERS_PAGE_SIZE & (PULSEEDITOR_WINNERS_PAGE_SIZE - 1)) == 0);
/// Maximum supported code length; each ticket uses at most this many digits.
constexpr uint8 PULSEEDITOR_MAX_CODE_LENGTH = 10;
/// QPI-aligned digit storage capacity for ticket and result arrays.
constexpr uint8 PULSEEDITOR_DIGITS_ALIGNED = pulseEditorNextPowerOfTwo(PULSEEDITOR_MAX_CODE_LENGTH);
/// Maximum allowed digit value; default range is `0..9`.
constexpr uint8 PULSEEDITOR_MAX_DIGIT = 9;
/// Bucket count used for digit frequency arrays; must cover `0..PULSEEDITOR_MAX_DIGIT` and be 2^N.
constexpr uint8 PULSEEDITOR_DIGIT_BUCKETS = 16;
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
	 * @brief Converts a typed return code into the compact public ABI representation.
	 * @param code Internal enum value.
	 * @return `uint8` value returned by public procedures and functions.
	 */
	static constexpr uint8 toReturnCode(const EReturnCode& code) { return static_cast<uint8>(code); }

	struct GameTemplate
	{
		Array<uint64, PULSEEDITOR_PAYOUT_MATRIX_CAPACITY> payoutMatrix;
		Array<uint8, 32> name;
		id owner;
		uint64 ticketPrice;
		uint64 prizeReserve;
		uint64 creatorRevenue;
		uint64 burnAccrued;
		uint64 totalRevenue;
		uint64 totalPaid;
		uint64 maxSinglePayout;
		uint32 currentRoundId;
		uint16 ticketLimit;
		uint16 playerTicketLimit;
		uint8 codeLength;
		uint8 maxDigit;
		uint8 creatorFeePercent;
		uint8 burnPercent;
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
		uint8 platformFeePercent;
		uint8 maxCreatorFeePercent;
	};

	/**
	 * @brief Input for creating a draft game template.
	 * @param payoutMatrix Fixed payout table indexed by `(exact, misplaced)`.
	 * @param name Creator-provided display name stored as raw bytes.
	 * @param ticketPrice Exact Qubic amount required for one ticket.
	 * @param ticketLimit Maximum tickets accepted by each round of this template.
	 * @param playerTicketLimit Maximum tickets one player may buy in a round.
	 * @param codeLength Number of digits players must submit.
	 * @param maxDigit Maximum allowed digit value; valid range is `0..maxDigit`.
	 * @param creatorFeePercent Creator share of non-platform ticket revenue.
	 * @param burnPercent Burn share of non-platform ticket revenue.
	 * @param allowRepeatedDigits Whether the same digit may appear more than once in a code.
	 */
	struct CreateTemplate_input
	{
		Array<uint64, PULSEEDITOR_PAYOUT_MATRIX_CAPACITY> payoutMatrix;
		Array<uint8, 32> name;
		uint64 ticketPrice;
		uint16 ticketLimit;
		uint16 playerTicketLimit;
		uint8 codeLength;
		uint8 maxDigit;
		uint8 creatorFeePercent;
		uint8 burnPercent;
		bit allowRepeatedDigits;
	};

	/**
	 * @brief Output from template creation.
	 * @param requiredPrizeReserve Minimum base reserve required before publication.
	 * @param templateId New template index when creation succeeds.
	 * @param returnCode `SUCCESS` or the validation/storage error.
	 */
	struct CreateTemplate_output
	{
		uint64 requiredPrizeReserve;
		uint16 templateId;
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
	 * @note The invocation reward must equal the template ticket price exactly.
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
		uint32 roundId;
		uint16 ticketIndex;
		uint8 returnCode;
	};

	/**
	 * @brief Input for settling the current selling round.
	 * @param templateId Template whose active round should be settled.
	 */
	struct SettleRound_input
	{
		uint16 templateId;
	};

	/**
	 * @brief Output from settlement.
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
	 * @param amount Qubic amount to transfer to the template owner.
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
	 * @brief Input for manually opening the next round after settlement.
	 * @param templateId Published template owned by the invocator.
	 */
	struct StartNextRound_input
	{
		uint16 templateId;
	};

	/**
	 * @brief Output from manual next-round creation.
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
	 * @brief Input for querying a stored ticket by global index.
	 * @param ticketIndex Index previously returned by `BuyTicket`.
	 */
	struct GetTicket_input
	{
		uint16 ticketIndex;
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
		ValidateDigits_input validateInput;
		ValidateDigits_output validateOutput;
		uint64 i;
		uint16 templateId;
		uint64 payout;
	};

	struct DepositPrizeReserve_locals
	{
		GameTemplate gameTemplate;
		uint64 depositAmount;
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
		uint64 seed;
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

	struct GetWinners_locals
	{
		uint64 oldestCounter;
		uint64 sourceCounter;
		uint64 sourceIndex;
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
		REGISTER_USER_PROCEDURE(SettleRound, 5);
		REGISTER_USER_PROCEDURE(RequestStop, 6);
		REGISTER_USER_PROCEDURE(WithdrawCreatorRevenue, 7);
		REGISTER_USER_PROCEDURE(SetPlatformConfig, 8);
		REGISTER_USER_PROCEDURE(WithdrawPlatformRevenue, 9);
		REGISTER_USER_PROCEDURE(StartNextRound, 10);
		REGISTER_USER_PROCEDURE(TransferShareManagementRights, 11);

		REGISTER_USER_FUNCTION(GetTemplate, 1);
		REGISTER_USER_FUNCTION(GetRound, 2);
		REGISTER_USER_FUNCTION(GetTicket, 3);
		REGISTER_USER_FUNCTION(GetWinners, 4);
		REGISTER_USER_FUNCTION(GetPlatformAccounting, 5);
		REGISTER_USER_FUNCTION(ValidateDigits, 6);
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
		if (!input.allowRepeatedDigits && input.codeLength > static_cast<uint8>(input.maxDigit + 1))
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_VALUE);
			return;
		}

		locals.templateId = static_cast<uint16>(state.get().templateCount);
		locals.gameTemplate.name = input.name;
		locals.gameTemplate.payoutMatrix = input.payoutMatrix;
		locals.gameTemplate.owner = qpi.invocator();
		locals.gameTemplate.ticketPrice = input.ticketPrice;
		locals.gameTemplate.ticketLimit = input.ticketLimit;
		locals.gameTemplate.playerTicketLimit = input.playerTicketLimit;
		locals.gameTemplate.codeLength = input.codeLength;
		locals.gameTemplate.maxDigit = input.maxDigit;
		locals.gameTemplate.creatorFeePercent = input.creatorFeePercent;
		locals.gameTemplate.burnPercent = input.burnPercent;
		locals.gameTemplate.allowRepeatedDigits = input.allowRepeatedDigits;
		locals.gameTemplate.status = ETemplateStatus::DRAFT;

		for (locals.i = 0; locals.i < locals.gameTemplate.payoutMatrix.capacity(); ++locals.i)
		{
			locals.payout = locals.gameTemplate.payoutMatrix.get(locals.i);
			if (locals.payout > locals.gameTemplate.maxSinglePayout)
			{
				locals.gameTemplate.maxSinglePayout = locals.payout;
			}
		}

		state.mut().templates.set(locals.templateId, locals.gameTemplate);
		state.mut().templateCount = sadd(state.get().templateCount, 1ULL);

		output.templateId = locals.templateId;
		output.requiredPrizeReserve = locals.gameTemplate.maxSinglePayout;
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
	 * @brief Publishes a funded draft template and opens its first selling round.
	 * @param input Template id to publish.
	 * @param output First round id.
	 * @return `SUCCESS`, `INVALID_TEMPLATE`, `ACCESS_DENIED`, `INVALID_STATE`, or `INSUFFICIENT_FUNDS`.
	 * @warning Publication requires the prize reserve to cover at least the maximum single-player payout.
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
		if (locals.gameTemplate.prizeReserve < locals.gameTemplate.maxSinglePayout)
		{
			output.returnCode = toReturnCode(EReturnCode::INSUFFICIENT_FUNDS);
			return;
		}

		locals.gameTemplate.status = ETemplateStatus::PUBLISHED;
		locals.gameTemplate.currentRoundId = sadd(locals.gameTemplate.currentRoundId, 1U);

		locals.round.roundId = locals.gameTemplate.currentRoundId;
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
		if (locals.reward != locals.gameTemplate.ticketPrice)
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
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
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
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
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
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			output.returnCode = toReturnCode(EReturnCode::PLAYER_TICKET_LIMIT);
			return;
		}

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
		++locals.round.ticketCount;

		locals.ticket.digits = input.digits;
		locals.ticket.player = qpi.invocator();
		locals.ticket.roundId = locals.round.roundId;
		locals.ticket.templateId = input.templateId;
		locals.ticket.status = ETicketStatus::ACTIVE;

		output.ticketIndex = static_cast<uint16>(state.get().ticketCount);
		output.roundId = locals.round.roundId;

		state.mut().tickets.set(state.get().ticketCount, locals.ticket);
		state.mut().ticketCount = sadd(state.get().ticketCount, 1ULL);
		state.mut().templates.set(input.templateId, locals.gameTemplate);
		state.mut().rounds.set(input.templateId, locals.round);

		output.returnCode = toReturnCode(EReturnCode::SUCCESS);
	}

	/**
	 * @brief Settles the active selling round by generating a winning code and paying fixed rewards.
	 * @param input Template id whose current round should be settled.
	 * @param output Winning digits, paid total, winner counts, and status code.
	 * @return `SUCCESS`, `INVALID_TEMPLATE`, `ACCESS_DENIED`, or `INVALID_STATE`.
	 * @note Only the template owner or platform owner may settle a round.
	 * @warning Winners are marked unpaid if the prize reserve cannot cover their fixed payout.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(SettleRound)
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

		if (qpi.invocator() != locals.gameTemplate.owner && qpi.invocator() != state.get().platformOwner)
		{
			output.returnCode = toReturnCode(EReturnCode::ACCESS_DENIED);
			return;
		}
		if (locals.round.status != ERoundStatus::SELLING)
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_STATE);
			return;
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
			locals.ticket.payout = locals.payout;

			if (locals.payout > 0)
			{
				++locals.round.winnerCount;
				if (locals.gameTemplate.prizeReserve >= locals.payout)
				{
					qpi.transfer(locals.ticket.player, locals.payout);
					locals.gameTemplate.prizeReserve = locals.gameTemplate.prizeReserve - locals.payout;
					locals.gameTemplate.totalPaid = sadd(locals.gameTemplate.totalPaid, locals.payout);
					locals.round.paid = sadd(locals.round.paid, locals.payout);
					locals.ticket.status = ETicketStatus::PAID;

					locals.winnerInfo.epoch = qpi.epoch();
					locals.winnerInfo.tick = qpi.tick();
					locals.winnerInfo.player = locals.ticket.player;
					locals.winnerInfo.payout = locals.payout;
					locals.winnerInfo.roundId = locals.round.roundId;
					locals.winnerInfo.templateId = input.templateId;
					locals.winnerInfo.exact = locals.ticket.exact;
					locals.winnerInfo.misplaced = locals.ticket.misplaced;
					locals.winnerIndex = mod(state.get().winnerCounter, state.get().winners.capacity());
					state.mut().winners.set(locals.winnerIndex, locals.winnerInfo);
					state.mut().winnerCounter = sadd(state.get().winnerCounter, 1ULL);
				}
				else
				{
					++locals.round.unpaidWinnerCount;
					locals.ticket.status = ETicketStatus::UNPAID;
				}
			}
			else
			{
				locals.ticket.status = ETicketStatus::PAID;
			}

			state.mut().tickets.set(locals.i, locals.ticket);
		}

		locals.round.status = ERoundStatus::SETTLED;
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
		if (locals.round.status == ERoundStatus::SELLING)
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
	 * @brief Opens the next manual round for a published template after the previous round settled.
	 * @param input Template id owned by the invocator.
	 * @param output New round id and status code.
	 * @return `SUCCESS`, `INVALID_TEMPLATE`, `ACCESS_DENIED`, `INVALID_STATE`, or `INSUFFICIENT_FUNDS`.
	 * @note This is the MVP replacement for automatic recurring scheduling.
	 */
	PUBLIC_PROCEDURE_WITH_LOCALS(StartNextRound)
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
		if (locals.gameTemplate.status != ETemplateStatus::PUBLISHED || locals.round.status == ERoundStatus::SELLING)
		{
			output.returnCode = toReturnCode(EReturnCode::INVALID_STATE);
			return;
		}
		if (locals.gameTemplate.prizeReserve < locals.gameTemplate.maxSinglePayout)
		{
			output.returnCode = toReturnCode(EReturnCode::INSUFFICIENT_FUNDS);
			return;
		}

		locals.gameTemplate.currentRoundId = sadd(locals.gameTemplate.currentRoundId, 1U);
		setMemory(locals.round, 0);
		locals.round.roundId = locals.gameTemplate.currentRoundId;
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
