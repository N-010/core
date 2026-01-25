#define NO_UEFI

#include "contract_testing.h"
#include <algorithm>
#include <vector>

static constexpr uint16 FUNCTION_INDEX_GET_RANDOM = 1;
static constexpr uint16 FUNCTION_INDEX_GET_ROUND_INFO = 2;
static constexpr uint16 FUNCTION_INDEX_GET_CURRENT_ROUND = 3;

static constexpr uint16 PROCEDURE_INDEX_COMMIT = 1;
static constexpr uint16 PROCEDURE_INDEX_REVEAL = 2;
static constexpr uint16 PROCEDURE_INDEX_GET_RANDOM_WITH_FEE = 3;

class RBeaconChecker : public RBEACON
{
public:
	void restoreRevealedOperators(const std::vector<id>& operators)
	{
		revealedOperators.reset();
		for (const auto& op : operators)
		{
			revealedOperators.add(op);
		}
	}

	uint64 getTreasury() const { return treasury; }
	uint32 getCurrentRoundId() const { return currentRoundId; }
	uint16 getRevealedCount() const { return revealedCount; }
	uint64 getCommitCount() const { return commits.population(); }
	uint64 getRevealedOperatorsCount() const { return revealedOperators.population(); }
	uint64 getRoundCount() const { return rounds.population(); }
	m256i getPreviousRoundRandom() const { return previousRoundRandom; }
	bool hasCommit(const id& operatorAddr) const { return commits.contains(operatorAddr); }
	bool getCommit(const id& operatorAddr, OperatorCommit& out) const { return commits.get(operatorAddr, out); }
	bool hasRevealedOperator(const id& operatorAddr) const { return revealedOperators.contains(operatorAddr); }

	void insertRound(uint32 roundId, const m256i& roundRandom, uint16 operatorCount, uint16 revealedCount, bool finalized,
	                 bool rewardsDistributed = false)
	{
		RoundData data{};
		data.roundId = roundId;
		data.roundRandom = roundRandom;
		data.rewardPool = 0;
		data.operatorCount = operatorCount;
		data.revealedCount = revealedCount;
		data.finalized = finalized;
		data.rewardsDistributed = rewardsDistributed;
		rounds.set(roundId, data);
	}

	static bool isScoreLess(const m256i& a, const m256i& b) { return RBEACON::isScoreLess(a, b); }
};

class ContractTestingRBeacon : public ContractTesting
{
public:
	ContractTestingRBeacon()
	{
		initEmptySpectrum();
		initEmptyUniverse();
		INIT_CONTRACT(RBEACON);
		callSystemProcedure(RBEACON_CONTRACT_INDEX, INITIALIZE);
		system.tick = 0;
	}

	RBeaconChecker* state() { return reinterpret_cast<RBeaconChecker*>(contractStates[RBEACON_CONTRACT_INDEX]); }

	void beginTick() { callSystemProcedure(RBEACON_CONTRACT_INDEX, BEGIN_TICK); }

	void setTick(uint32 tick, bool runBeginTick)
	{
		system.tick = tick;
		if (runBeginTick)
		{
			beginTick();
		}
	}

	RBEACON::Commit_output commit(const id& user, const id& seed, const id& salt, uint32 roundId)
	{
		RBEACON::Commit_input input{};
		input.commitHash = computeCommitHash(seed, salt, roundId);
		RBEACON::Commit_output output{};
		invokeUserProcedure(RBEACON_CONTRACT_INDEX, PROCEDURE_INDEX_COMMIT, input, output, user, RBEACON_DEPOSIT);
		return output;
	}

	RBEACON::Reveal_output reveal(const id& user, const id& seed, const id& salt)
	{
		RBEACON::Reveal_input input{};
		input.seed = seed;
		input.salt = salt;
		RBEACON::Reveal_output output{};
		invokeUserProcedure(RBEACON_CONTRACT_INDEX, PROCEDURE_INDEX_REVEAL, input, output, user, 0);
		return output;
	}

	RBEACON::GetRandom_output getRandom(uint32 roundId)
	{
		RBEACON::GetRandom_input input{};
		input.roundId = roundId;
		RBEACON::GetRandom_output output{};
		callFunction(RBEACON_CONTRACT_INDEX, FUNCTION_INDEX_GET_RANDOM, input, output);
		return output;
	}

	RBEACON::GetRoundInfo_output getRoundInfo(uint32 roundId)
	{
		RBEACON::GetRoundInfo_input input{};
		input.roundId = roundId;
		RBEACON::GetRoundInfo_output output{};
		callFunction(RBEACON_CONTRACT_INDEX, FUNCTION_INDEX_GET_ROUND_INFO, input, output);
		return output;
	}

	RBEACON::GetRandomWithFee_output getRandomWithFee(const id& invocator, uint32 roundId, uint64 fee)
	{
		RBEACON::GetRandomWithFee_input input{};
		input.roundId = roundId;
		RBEACON::GetRandomWithFee_output output{};
		invokeUserProcedure(RBEACON_CONTRACT_INDEX, PROCEDURE_INDEX_GET_RANDOM_WITH_FEE, input, output, invocator, fee);
		return output;
	}

	RBEACON::GetCurrentRound_output getCurrentRound()
	{
		RBEACON::GetCurrentRound_input input{};
		RBEACON::GetCurrentRound_output output{};
		callFunction(RBEACON_CONTRACT_INDEX, FUNCTION_INDEX_GET_CURRENT_ROUND, input, output);
		return output;
	}

	static id computeCommitHashForTest(const id& seed, const id& salt, uint32 roundId) { return computeCommitHash(seed, salt, roundId); }

private:
	static id computeCommitHash(const id& seed, const id& salt, uint32 roundId)
	{
		RBEACON::CommitHashInput input{};
		input.seed = seed;
		input.salt = salt;
		input.round = id::zero();
		input.round.u64._0 = roundId;
		id out{};
		KangarooTwelve(&input, sizeof(input), &out, sizeof(out));
		return out;
	}
};

static id makeUser()
{
	return id::randomValue();
}

static id makeSeed()
{
	return id::randomValue();
}

static id makeSalt()
{
	return id::randomValue();
}

static m256i xorSeeds(const std::vector<id>& seeds)
{
	m256i combined = id::zero();
	for (const auto& seed : seeds)
	{
		combined.u64._0 ^= seed.u64._0;
		combined.u64._1 ^= seed.u64._1;
		combined.u64._2 ^= seed.u64._2;
		combined.u64._3 ^= seed.u64._3;
	}
	return combined;
}

static m256i computeWinnerScore(const m256i& roundRandom, const id& operatorAddr)
{
	RBEACON::WinnerHashInput input{};
	input.random = roundRandom;
	input.operatorAddr = operatorAddr;
	m256i out{};
	KangarooTwelve(&input, sizeof(input), &out, sizeof(out));
	return out;
}

TEST(ContractRBeacon, InitializeStateDefaults)
{
	ContractTestingRBeacon rb;

	auto* state = rb.state();
	EXPECT_EQ(state->getCurrentRoundId(), 0U);
	EXPECT_EQ(state->getTreasury(), 0ULL);
	EXPECT_EQ(state->getPreviousRoundRandom(), NULL_ID);
	EXPECT_EQ(state->getCommitCount(), 0ULL);
	EXPECT_EQ(state->getRevealedOperatorsCount(), 0ULL);
	EXPECT_EQ(static_cast<uint32>(state->getRevealedCount()), 0U);
	EXPECT_EQ(state->getRoundCount(), 0ULL);
}

TEST(ContractRBeacon, CommitStoresRecord)
{
	ContractTestingRBeacon rb;

	rb.setTick(10, true);

	const id operatorAddr = makeUser();
	const id seed = makeSeed();
	const id salt = makeSalt();
	increaseEnergy(operatorAddr, RBEACON_DEPOSIT);

	const auto commitOut = rb.commit(operatorAddr, seed, salt, 0);
	EXPECT_EQ(commitOut.returnCode, static_cast<uint8>(RBEACON::EReturnCode::SUCCESS));
	EXPECT_EQ(commitOut.roundId, 0U);

	auto* state = rb.state();
	EXPECT_EQ(state->getCommitCount(), 1ULL);
	EXPECT_TRUE(state->hasCommit(operatorAddr));
	RBEACON::OperatorCommit record{};
	ASSERT_TRUE(state->getCommit(operatorAddr, record));
	EXPECT_EQ(record.commitHash, ContractTestingRBeacon::computeCommitHashForTest(seed, salt, 0));
	EXPECT_EQ(record.seed, NULL_ID);
	EXPECT_EQ(record.deposit, RBEACON_DEPOSIT);
	EXPECT_FALSE(record.revealed);
	EXPECT_EQ(state->getRevealedOperatorsCount(), 0ULL);
	EXPECT_EQ(static_cast<uint32>(state->getRevealedCount()), 0U);
}

TEST(ContractRBeacon, RevealSuccessUpdatesState)
{
	ContractTestingRBeacon rb;

	rb.setTick(10, true);

	const id operatorAddr = makeUser();
	const id seed = makeSeed();
	const id salt = makeSalt();
	increaseEnergy(operatorAddr, RBEACON_DEPOSIT);
	const uint64 balanceBeforeCommit = getBalance(operatorAddr);

	const auto commitOut = rb.commit(operatorAddr, seed, salt, 0);
	EXPECT_EQ(commitOut.returnCode, static_cast<uint8>(RBEACON::EReturnCode::SUCCESS));
	const uint64 balanceAfterCommit = getBalance(operatorAddr);
	EXPECT_EQ(balanceBeforeCommit - balanceAfterCommit, RBEACON_DEPOSIT);

	rb.setTick(60, false);
	const uint64 balanceBeforeReveal = getBalance(operatorAddr);
	const auto revealOut = rb.reveal(operatorAddr, seed, salt);
	EXPECT_EQ(revealOut.returnCode, static_cast<uint8>(RBEACON::EReturnCode::SUCCESS));
	EXPECT_EQ(getBalance(operatorAddr), balanceBeforeReveal + RBEACON_DEPOSIT);

	auto* state = rb.state();
	RBEACON::OperatorCommit record{};
	ASSERT_TRUE(state->getCommit(operatorAddr, record));
	EXPECT_EQ(record.seed, seed);
	EXPECT_TRUE(record.revealed);
	EXPECT_TRUE(state->hasRevealedOperator(operatorAddr));
	EXPECT_EQ(static_cast<uint32>(state->getRevealedCount()), 1U);
	EXPECT_EQ(state->getRevealedOperatorsCount(), 1ULL);
}

TEST(ContractRBeacon, RevealInvalidPhaseDoesNotChangeRecord)
{
	ContractTestingRBeacon rb;

	rb.setTick(10, true);

	const id operatorAddr = makeUser();
	const id seed = makeSeed();
	const id salt = makeSalt();
	increaseEnergy(operatorAddr, RBEACON_DEPOSIT);

	const auto commitOut = rb.commit(operatorAddr, seed, salt, 0);
	EXPECT_EQ(commitOut.returnCode, static_cast<uint8>(RBEACON::EReturnCode::SUCCESS));

	auto* state = rb.state();
	RBEACON::OperatorCommit before{};
	ASSERT_TRUE(state->getCommit(operatorAddr, before));
	const uint64 balanceBefore = getBalance(operatorAddr);

	const auto revealOut = rb.reveal(operatorAddr, seed, salt);
	EXPECT_EQ(revealOut.returnCode, static_cast<uint8>(RBEACON::EReturnCode::INVALID_PHASE));
	EXPECT_EQ(getBalance(operatorAddr), balanceBefore);

	RBEACON::OperatorCommit after{};
	ASSERT_TRUE(state->getCommit(operatorAddr, after));
	EXPECT_EQ(after.commitHash, before.commitHash);
	EXPECT_EQ(after.seed, before.seed);
	EXPECT_EQ(after.deposit, before.deposit);
	EXPECT_EQ(after.revealed, before.revealed);
	EXPECT_EQ(static_cast<uint32>(state->getRevealedCount()), 0U);
	EXPECT_EQ(state->getRevealedOperatorsCount(), 0ULL);
}

TEST(ContractRBeacon, CommitRevealFinalize)
{
	ContractTestingRBeacon rb;

	static constexpr uint32 roundId = 1;
	rb.setTick(100, true);

	std::vector<id> operators;
	std::vector<id> seeds;
	std::vector<id> salts;
	for (uint64 i = 0; i < 2; ++i)
	{
		operators.push_back(makeUser());
		seeds.push_back(makeSeed());
		salts.push_back(makeSalt());
		increaseEnergy(operators.back(), RBEACON_DEPOSIT);
		const RBEACON::Commit_output& commitOut = rb.commit(operators.back(), seeds.back(), salts.back(), roundId);
		EXPECT_EQ(commitOut.returnCode, static_cast<uint8>(RBEACON::EReturnCode::SUCCESS));
	}

	rb.setTick(150, false);
	for (size_t i = 0; i < operators.size(); ++i)
	{
		auto revealOut = rb.reveal(operators[i], seeds[i], salts[i]);
		EXPECT_EQ(revealOut.returnCode, static_cast<uint8>(RBEACON::EReturnCode::SUCCESS));
	}
	EXPECT_EQ(static_cast<uint32>(rb.state()->getRevealedCount()), operators.size());
	EXPECT_EQ(rb.state()->getRevealedOperatorsCount(), operators.size());

	rb.setTick(200, true);
	EXPECT_EQ(rb.state()->getCommitCount(), 0ULL);
	EXPECT_EQ(rb.state()->getRevealedOperatorsCount(), 0ULL);
	EXPECT_EQ(static_cast<uint32>(rb.state()->getRevealedCount()), 0U);
	EXPECT_EQ(rb.state()->getCurrentRoundId(), 2U);
	EXPECT_EQ(rb.state()->getRoundCount(), 1ULL);
	EXPECT_EQ(rb.state()->getTreasury(), 0ULL);

	auto roundInfo = rb.getRoundInfo(roundId);
	EXPECT_EQ(roundInfo.returnCode, static_cast<uint8>(RBEACON::EReturnCode::SUCCESS));
	EXPECT_TRUE(roundInfo.finalized);
	EXPECT_EQ(static_cast<uint32>(roundInfo.operatorCount), operators.size());
	EXPECT_EQ(static_cast<uint32>(roundInfo.revealedCount), operators.size());

	const m256i expectedRandom = xorSeeds(seeds);
	EXPECT_EQ(roundInfo.roundRandom, expectedRandom);
	EXPECT_EQ(rb.state()->getPreviousRoundRandom(), expectedRandom);

	auto randomOut = rb.getRandom(roundId);
	EXPECT_EQ(randomOut.returnCode, static_cast<uint8>(RBEACON::EReturnCode::SUCCESS));
	EXPECT_EQ(randomOut.randomValue, expectedRandom);
}

TEST(ContractRBeacon, InsufficientReveals)
{
	ContractTestingRBeacon rb;

	static constexpr uint32 roundId = 1;
	rb.setTick(100, true);

	const id operatorAddr = makeUser();
	const id seed = makeSeed();
	const id salt = makeSalt();

	increaseEnergy(operatorAddr, RBEACON_DEPOSIT);
	auto commitOut = rb.commit(operatorAddr, seed, salt, roundId);
	EXPECT_EQ(commitOut.returnCode, static_cast<uint8>(RBEACON::EReturnCode::SUCCESS));

	rb.setTick(150, false);
	auto revealOut = rb.reveal(operatorAddr, seed, salt);
	EXPECT_EQ(revealOut.returnCode, static_cast<uint8>(RBEACON::EReturnCode::SUCCESS));

	rb.setTick(200, true);
	EXPECT_EQ(rb.state()->getCommitCount(), 0ULL);
	EXPECT_EQ(rb.state()->getRevealedOperatorsCount(), 0ULL);
	EXPECT_EQ(static_cast<uint32>(rb.state()->getRevealedCount()), 0U);
	EXPECT_EQ(rb.state()->getPreviousRoundRandom(), NULL_ID);
	EXPECT_EQ(rb.state()->getTreasury(), 0ULL);

	auto randomOut = rb.getRandom(roundId);
	EXPECT_EQ(randomOut.returnCode, static_cast<uint8>(RBEACON::EReturnCode::INSUFFICIENT_REVEALS));

	const id caller = makeUser();
	static constexpr uint64 fee = 100000;
	increaseEnergy(caller, fee);
	const uint64 balanceBefore = getBalance(caller);
	auto randomWithFeeOut = rb.getRandomWithFee(caller, roundId, fee);
	EXPECT_EQ(randomWithFeeOut.returnCode, static_cast<uint8>(RBEACON::EReturnCode::INSUFFICIENT_REVEALS));
	EXPECT_EQ(getBalance(caller), balanceBefore);

	auto roundInfo = rb.getRoundInfo(roundId);
	EXPECT_EQ(roundInfo.returnCode, static_cast<uint8>(RBEACON::EReturnCode::SUCCESS));
	EXPECT_EQ(roundInfo.rewardPool, 0ULL);
	EXPECT_FALSE(roundInfo.rewardsDistributed);
}

TEST(ContractRBeacon, CommitInvalidPhaseRefunds)
{
	ContractTestingRBeacon rb;

	rb.setTick(50, true);

	const id operatorAddr = makeUser();
	const id seed = makeSeed();
	const id salt = makeSalt();
	increaseEnergy(operatorAddr, RBEACON_DEPOSIT);
	const uint64 balanceBefore = getBalance(operatorAddr);

	auto commitOut = rb.commit(operatorAddr, seed, salt, 0);
	EXPECT_EQ(commitOut.returnCode, static_cast<uint8>(RBEACON::EReturnCode::INVALID_PHASE));
	EXPECT_EQ(getBalance(operatorAddr), balanceBefore);
	EXPECT_EQ(rb.state()->getCommitCount(), 0ULL);
}

TEST(ContractRBeacon, CommitInsufficientDepositRefunds)
{
	ContractTestingRBeacon rb;

	rb.setTick(10, true);

	const id operatorAddr = makeUser();
	const id seed = makeSeed();
	const id salt = makeSalt();
	static constexpr uint64 deposit = RBEACON_DEPOSIT - 1;
	increaseEnergy(operatorAddr, deposit);
	const uint64 balanceBefore = getBalance(operatorAddr);

	RBEACON::Commit_input input{};
	input.commitHash = ContractTestingRBeacon::computeCommitHashForTest(seed, salt, 0);
	RBEACON::Commit_output output{};
	rb.invokeUserProcedure(RBEACON_CONTRACT_INDEX, PROCEDURE_INDEX_COMMIT, input, output, operatorAddr, deposit);
	EXPECT_EQ(output.returnCode, static_cast<uint8>(RBEACON::EReturnCode::INSUFFICIENT_DEPOSIT));
	EXPECT_EQ(getBalance(operatorAddr), balanceBefore);
	EXPECT_EQ(rb.state()->getCommitCount(), 0ULL);
}

TEST(ContractRBeacon, CommitAlreadyCommittedRefunds)
{
	ContractTestingRBeacon rb;

	rb.setTick(10, true);

	const id operatorAddr = makeUser();
	const id seed = makeSeed();
	const id salt = makeSalt();
	increaseEnergy(operatorAddr, RBEACON_DEPOSIT * 2);
	const uint64 balanceBefore = getBalance(operatorAddr);

	auto firstCommit = rb.commit(operatorAddr, seed, salt, 0);
	EXPECT_EQ(firstCommit.returnCode, static_cast<uint8>(RBEACON::EReturnCode::SUCCESS));
	const uint64 balanceAfterFirst = getBalance(operatorAddr);
	RBEACON::OperatorCommit recordBefore{};
	ASSERT_TRUE(rb.state()->getCommit(operatorAddr, recordBefore));

	auto secondCommit = rb.commit(operatorAddr, seed, salt, 0);
	EXPECT_EQ(secondCommit.returnCode, static_cast<uint8>(RBEACON::EReturnCode::ALREADY_COMMITTED));
	EXPECT_EQ(getBalance(operatorAddr), balanceAfterFirst);
	EXPECT_EQ(balanceBefore - balanceAfterFirst, RBEACON_DEPOSIT);
	EXPECT_EQ(rb.state()->getCommitCount(), 1ULL);
	RBEACON::OperatorCommit recordAfter{};
	ASSERT_TRUE(rb.state()->getCommit(operatorAddr, recordAfter));
	EXPECT_EQ(recordAfter.commitHash, recordBefore.commitHash);
	EXPECT_EQ(recordAfter.seed, recordBefore.seed);
	EXPECT_EQ(recordAfter.deposit, recordBefore.deposit);
	EXPECT_EQ(recordAfter.revealed, recordBefore.revealed);
}

TEST(ContractRBeacon, CommitMaxOperatorsReached)
{
	ContractTestingRBeacon rb;

	rb.setTick(10, true);

	static constexpr uint32 roundId = 0;
	for (uint32 i = 0; i < RBEACON_MAX_OPERATORS; ++i)
	{
		const id operatorAddr = makeUser();
		const id seed = makeSeed();
		const id salt = makeSalt();
		increaseEnergy(operatorAddr, RBEACON_DEPOSIT);
		auto commitOut = rb.commit(operatorAddr, seed, salt, roundId);
		EXPECT_EQ(commitOut.returnCode, static_cast<uint8>(RBEACON::EReturnCode::SUCCESS));
	}

	const id extraOperator = makeUser();
	const id extraSeed = makeSeed();
	const id extraSalt = makeSalt();
	increaseEnergy(extraOperator, RBEACON_DEPOSIT);
	const uint64 balanceBefore = getBalance(extraOperator);
	auto extraCommit = rb.commit(extraOperator, extraSeed, extraSalt, roundId);
	EXPECT_EQ(extraCommit.returnCode, static_cast<uint8>(RBEACON::EReturnCode::MAX_OPERATORS_REACHED));
	EXPECT_EQ(getBalance(extraOperator), balanceBefore);
	EXPECT_EQ(rb.state()->getCommitCount(), RBEACON_MAX_OPERATORS);
	EXPECT_FALSE(rb.state()->hasCommit(extraOperator));
}

TEST(ContractRBeacon, RevealInvalidPhase)
{
	ContractTestingRBeacon rb;

	rb.setTick(10, true);

	const id operatorAddr = makeUser();
	const id seed = makeSeed();
	const id salt = makeSalt();
	increaseEnergy(operatorAddr, 1);
	auto revealOut = rb.reveal(operatorAddr, seed, salt);
	EXPECT_EQ(revealOut.returnCode, static_cast<uint8>(RBEACON::EReturnCode::INVALID_PHASE));
	EXPECT_EQ(rb.state()->getCommitCount(), 0ULL);
	EXPECT_EQ(static_cast<uint32>(rb.state()->getRevealedCount()), 0U);
	EXPECT_EQ(rb.state()->getRevealedOperatorsCount(), 0ULL);
}

TEST(ContractRBeacon, RevealNotCommitted)
{
	ContractTestingRBeacon rb;

	rb.setTick(60, true);

	const id operatorAddr = makeUser();
	const id seed = makeSeed();
	const id salt = makeSalt();
	increaseEnergy(operatorAddr, 1);
	auto revealOut = rb.reveal(operatorAddr, seed, salt);
	EXPECT_EQ(revealOut.returnCode, static_cast<uint8>(RBEACON::EReturnCode::NOT_COMMITTED));
	EXPECT_EQ(rb.state()->getCommitCount(), 0ULL);
	EXPECT_EQ(static_cast<uint32>(rb.state()->getRevealedCount()), 0U);
	EXPECT_EQ(rb.state()->getRevealedOperatorsCount(), 0ULL);
}

TEST(ContractRBeacon, RevealAlreadyRevealed)
{
	ContractTestingRBeacon rb;

	rb.setTick(10, true);

	const id operatorAddr = makeUser();
	const id seed = makeSeed();
	const id salt = makeSalt();
	increaseEnergy(operatorAddr, RBEACON_DEPOSIT);
	auto commitOut = rb.commit(operatorAddr, seed, salt, 0);
	EXPECT_EQ(commitOut.returnCode, static_cast<uint8>(RBEACON::EReturnCode::SUCCESS));

	rb.setTick(60, false);
	auto firstReveal = rb.reveal(operatorAddr, seed, salt);
	EXPECT_EQ(firstReveal.returnCode, static_cast<uint8>(RBEACON::EReturnCode::SUCCESS));
	const uint64 balanceAfterFirst = getBalance(operatorAddr);
	EXPECT_EQ(static_cast<uint32>(rb.state()->getRevealedCount()), 1U);

	auto secondReveal = rb.reveal(operatorAddr, seed, salt);
	EXPECT_EQ(secondReveal.returnCode, static_cast<uint8>(RBEACON::EReturnCode::ALREADY_REVEALED));
	EXPECT_EQ(getBalance(operatorAddr), balanceAfterFirst);
	EXPECT_EQ(static_cast<uint32>(rb.state()->getRevealedCount()), 1U);
	EXPECT_EQ(rb.state()->getRevealedOperatorsCount(), 1ULL);
}

TEST(ContractRBeacon, RevealHashMismatchRemovesCommit)
{
	ContractTestingRBeacon rb;

	rb.setTick(10, true);

	const id operatorAddr = makeUser();
	const id seed = makeSeed();
	const id salt = makeSalt();
	const id wrongSalt = makeSalt();
	increaseEnergy(operatorAddr, RBEACON_DEPOSIT);
	const uint64 balanceBefore = getBalance(operatorAddr);
	auto commitOut = rb.commit(operatorAddr, seed, salt, 0);
	EXPECT_EQ(commitOut.returnCode, static_cast<uint8>(RBEACON::EReturnCode::SUCCESS));
	const uint64 balanceAfterCommit = getBalance(operatorAddr);

	rb.setTick(60, false);
	auto revealOut = rb.reveal(operatorAddr, seed, wrongSalt);
	EXPECT_EQ(revealOut.returnCode, static_cast<uint8>(RBEACON::EReturnCode::HASH_MISMATCH));
	EXPECT_EQ(getBalance(operatorAddr), balanceAfterCommit);
	EXPECT_EQ(balanceBefore - balanceAfterCommit, RBEACON_DEPOSIT);
	EXPECT_EQ(rb.state()->getCommitCount(), 0ULL);
	EXPECT_EQ(static_cast<uint32>(rb.state()->getRevealedCount()), 0U);
	EXPECT_EQ(rb.state()->getRevealedOperatorsCount(), 0ULL);

	auto secondReveal = rb.reveal(operatorAddr, seed, salt);
	EXPECT_EQ(secondReveal.returnCode, static_cast<uint8>(RBEACON::EReturnCode::NOT_COMMITTED));
}

TEST(ContractRBeacon, RevealHashMismatchWithWrongRoundId)
{
	ContractTestingRBeacon rb;

	rb.setTick(100, true);

	const id operatorAddr = makeUser();
	const id seed = makeSeed();
	const id salt = makeSalt();
	increaseEnergy(operatorAddr, RBEACON_DEPOSIT);
	const uint64 balanceBefore = getBalance(operatorAddr);

	RBEACON::Commit_input input{};
	input.commitHash = ContractTestingRBeacon::computeCommitHashForTest(seed, salt, 0);
	RBEACON::Commit_output commitOut{};
	rb.invokeUserProcedure(RBEACON_CONTRACT_INDEX, PROCEDURE_INDEX_COMMIT, input, commitOut, operatorAddr, RBEACON_DEPOSIT);
	EXPECT_EQ(commitOut.returnCode, static_cast<uint8>(RBEACON::EReturnCode::SUCCESS));
	const uint64 balanceAfterCommit = getBalance(operatorAddr);

	rb.setTick(150, false);
	auto revealOut = rb.reveal(operatorAddr, seed, salt);
	EXPECT_EQ(revealOut.returnCode, static_cast<uint8>(RBEACON::EReturnCode::HASH_MISMATCH));
	EXPECT_EQ(getBalance(operatorAddr), balanceAfterCommit);
	EXPECT_EQ(balanceBefore - balanceAfterCommit, RBEACON_DEPOSIT);
	EXPECT_EQ(rb.state()->getCommitCount(), 0ULL);
	EXPECT_EQ(static_cast<uint32>(rb.state()->getRevealedCount()), 0U);
	EXPECT_EQ(rb.state()->getRevealedOperatorsCount(), 0ULL);
}

TEST(ContractRBeacon, FinalizeForfeitsUnrevealedDeposit)
{
	ContractTestingRBeacon rb;

	static constexpr uint32 roundId = 1;
	rb.setTick(100, true);

	const id operatorA = makeUser();
	const id operatorB = makeUser();
	const id seedA = makeSeed();
	const id seedB = makeSeed();
	const id saltA = makeSalt();
	const id saltB = makeSalt();
	increaseEnergy(operatorA, RBEACON_DEPOSIT);
	increaseEnergy(operatorB, RBEACON_DEPOSIT);
	auto commitA = rb.commit(operatorA, seedA, saltA, roundId);
	auto commitB = rb.commit(operatorB, seedB, saltB, roundId);
	EXPECT_EQ(commitA.returnCode, static_cast<uint8>(RBEACON::EReturnCode::SUCCESS));
	EXPECT_EQ(commitB.returnCode, static_cast<uint8>(RBEACON::EReturnCode::SUCCESS));

	rb.setTick(150, false);
	auto revealA = rb.reveal(operatorA, seedA, saltA);
	EXPECT_EQ(revealA.returnCode, static_cast<uint8>(RBEACON::EReturnCode::SUCCESS));

	rb.setTick(200, true);
	EXPECT_EQ(rb.state()->getTreasury(), RBEACON_DEPOSIT);
	EXPECT_EQ(rb.state()->getCommitCount(), 0ULL);
	EXPECT_EQ(rb.state()->getRevealedOperatorsCount(), 0ULL);
	EXPECT_EQ(static_cast<uint32>(rb.state()->getRevealedCount()), 0U);
	EXPECT_EQ(rb.state()->getPreviousRoundRandom(), NULL_ID);

	auto roundInfo = rb.getRoundInfo(roundId);
	EXPECT_EQ(roundInfo.returnCode, static_cast<uint8>(RBEACON::EReturnCode::SUCCESS));
	EXPECT_EQ(static_cast<uint32>(roundInfo.operatorCount), 2U);
	EXPECT_EQ(static_cast<uint32>(roundInfo.revealedCount), 1U);
	EXPECT_TRUE(roundInfo.finalized);
	EXPECT_EQ(roundInfo.roundRandom, seedA);
}

TEST(ContractRBeacon, RoundNotFoundErrors)
{
	ContractTestingRBeacon rb;

	static constexpr uint32 missingRound = 999;
	auto randomOut = rb.getRandom(missingRound);
	EXPECT_EQ(randomOut.returnCode, static_cast<uint8>(RBEACON::EReturnCode::ROUND_NOT_FOUND));

	auto roundInfo = rb.getRoundInfo(missingRound);
	EXPECT_EQ(roundInfo.returnCode, static_cast<uint8>(RBEACON::EReturnCode::ROUND_NOT_FOUND));

	const id caller = makeUser();
	static constexpr uint64 fee = 1000;
	increaseEnergy(caller, fee);
	const uint64 balanceBefore = getBalance(caller);
	auto randomWithFeeOut = rb.getRandomWithFee(caller, missingRound, fee);
	EXPECT_EQ(randomWithFeeOut.returnCode, static_cast<uint8>(RBEACON::EReturnCode::ROUND_NOT_FOUND));
	EXPECT_EQ(getBalance(caller), balanceBefore);
	EXPECT_EQ(rb.state()->getTreasury(), 0ULL);
}

TEST(ContractRBeacon, GetRandomWithFeeRoundNotFinalized)
{
	ContractTestingRBeacon rb;

	static constexpr uint32 roundId = 7;
	rb.state()->insertRound(roundId, NULL_ID, 3, RBEACON_MIN_REVEALS, false);

	const id caller = makeUser();
	static constexpr uint64 fee = 5000;
	increaseEnergy(caller, fee);
	const uint64 balanceBefore = getBalance(caller);
	auto out = rb.getRandomWithFee(caller, roundId, fee);
	EXPECT_EQ(out.returnCode, static_cast<uint8>(RBEACON::EReturnCode::ROUND_NOT_FINALIZED));
	EXPECT_EQ(getBalance(caller), balanceBefore);
	auto roundInfo = rb.getRoundInfo(roundId);
	EXPECT_EQ(roundInfo.rewardPool, 0ULL);
	EXPECT_FALSE(roundInfo.rewardsDistributed);
	EXPECT_EQ(rb.state()->getTreasury(), 0ULL);
}

TEST(ContractRBeacon, GetRandomWithFeeInsufficientRevealsZero)
{
	ContractTestingRBeacon rb;

	static constexpr uint32 roundId = 9;
	rb.state()->insertRound(roundId, NULL_ID, 0, 0, true);

	const id caller = makeUser();
	static constexpr uint64 fee = 1000;
	increaseEnergy(caller, fee);
	const uint64 balanceBefore = getBalance(caller);
	auto out = rb.getRandomWithFee(caller, roundId, fee);
	EXPECT_EQ(out.returnCode, static_cast<uint8>(RBEACON::EReturnCode::INSUFFICIENT_REVEALS));
	EXPECT_EQ(getBalance(caller), balanceBefore);
	auto roundInfo = rb.getRoundInfo(roundId);
	EXPECT_EQ(roundInfo.rewardPool, 0ULL);
	EXPECT_FALSE(roundInfo.rewardsDistributed);
	EXPECT_EQ(rb.state()->getTreasury(), 0ULL);
}

TEST(ContractRBeacon, GetCurrentRoundPhaseBoundaries)
{
	ContractTestingRBeacon rb;

	rb.setTick(49, true);
	auto roundInfo = rb.getCurrentRound();
	EXPECT_EQ(roundInfo.roundId, 0u);
	EXPECT_EQ(roundInfo.roundTick, 49u);
	EXPECT_TRUE(roundInfo.isCommitPhase);
	EXPECT_FALSE(roundInfo.isRevealPhase);
	EXPECT_EQ(static_cast<uint32>(roundInfo.operatorCount), 0U);
	EXPECT_EQ(static_cast<uint32>(roundInfo.revealedCount), 0U);

	rb.setTick(50, true);
	roundInfo = rb.getCurrentRound();
	EXPECT_EQ(roundInfo.roundId, 0u);
	EXPECT_EQ(roundInfo.roundTick, 50u);
	EXPECT_FALSE(roundInfo.isCommitPhase);
	EXPECT_TRUE(roundInfo.isRevealPhase);

	rb.setTick(99, true);
	roundInfo = rb.getCurrentRound();
	EXPECT_EQ(roundInfo.roundId, 0u);
	EXPECT_EQ(roundInfo.roundTick, 99u);
	EXPECT_FALSE(roundInfo.isCommitPhase);
	EXPECT_TRUE(roundInfo.isRevealPhase);

	rb.setTick(100, true);
	roundInfo = rb.getCurrentRound();
	EXPECT_EQ(roundInfo.roundId, 1u);
	EXPECT_EQ(roundInfo.roundTick, 0u);
	EXPECT_TRUE(roundInfo.isCommitPhase);
	EXPECT_FALSE(roundInfo.isRevealPhase);
}

TEST(ContractRBeacon, GetCurrentRoundCountsAfterCommitReveal)
{
	ContractTestingRBeacon rb;

	rb.setTick(10, true);

	const id operatorAddr = makeUser();
	const id seed = makeSeed();
	const id salt = makeSalt();
	increaseEnergy(operatorAddr, RBEACON_DEPOSIT);
	auto commitOut = rb.commit(operatorAddr, seed, salt, 0);
	EXPECT_EQ(commitOut.returnCode, static_cast<uint8>(RBEACON::EReturnCode::SUCCESS));

	auto roundInfo = rb.getCurrentRound();
	EXPECT_EQ(static_cast<uint32>(roundInfo.operatorCount), 1U);
	EXPECT_EQ(static_cast<uint32>(roundInfo.revealedCount), 0U);

	rb.setTick(60, false);
	auto revealOut = rb.reveal(operatorAddr, seed, salt);
	EXPECT_EQ(revealOut.returnCode, static_cast<uint8>(RBEACON::EReturnCode::SUCCESS));

	roundInfo = rb.getCurrentRound();
	EXPECT_EQ(static_cast<uint32>(roundInfo.operatorCount), 1U);
	EXPECT_EQ(static_cast<uint32>(roundInfo.revealedCount), 1U);
}

TEST(ContractRBeacon, ReadOnlyFunctionsDoNotMutateState)
{
	ContractTestingRBeacon rb;

	rb.setTick(10, true);

	const id operatorAddr = makeUser();
	const id seed = makeSeed();
	const id salt = makeSalt();
	increaseEnergy(operatorAddr, RBEACON_DEPOSIT);
	auto commitOut = rb.commit(operatorAddr, seed, salt, 0);
	EXPECT_EQ(commitOut.returnCode, static_cast<uint8>(RBEACON::EReturnCode::SUCCESS));

	auto* state = rb.state();
	const uint64 commitCountBefore = state->getCommitCount();
	const uint64 revealedOpsBefore = state->getRevealedOperatorsCount();
	const uint16 revealedCountBefore = state->getRevealedCount();
	const uint64 treasuryBefore = state->getTreasury();
	const uint32 currentRoundBefore = state->getCurrentRoundId();
	const uint64 roundCountBefore = state->getRoundCount();
	const m256i prevRandomBefore = state->getPreviousRoundRandom();

	auto currentRound = rb.getCurrentRound();
	EXPECT_EQ(currentRound.roundId, currentRoundBefore);

	auto randomOut = rb.getRandom(42);
	EXPECT_EQ(randomOut.returnCode, static_cast<uint8>(RBEACON::EReturnCode::ROUND_NOT_FOUND));

	auto infoOut = rb.getRoundInfo(42);
	EXPECT_EQ(infoOut.returnCode, static_cast<uint8>(RBEACON::EReturnCode::ROUND_NOT_FOUND));

	EXPECT_EQ(state->getCommitCount(), commitCountBefore);
	EXPECT_EQ(state->getRevealedOperatorsCount(), revealedOpsBefore);
	EXPECT_EQ(static_cast<uint32>(state->getRevealedCount()), static_cast<uint32>(revealedCountBefore));
	EXPECT_EQ(state->getTreasury(), treasuryBefore);
	EXPECT_EQ(state->getCurrentRoundId(), currentRoundBefore);
	EXPECT_EQ(state->getRoundCount(), roundCountBefore);
	EXPECT_EQ(state->getPreviousRoundRandom(), prevRandomBefore);
	EXPECT_TRUE(state->hasCommit(operatorAddr));
}

TEST(ContractRBeacon, FinalizeWithNoReveals)
{
	ContractTestingRBeacon rb;

	static constexpr uint32 roundId = 1;
	rb.setTick(100, true);
	rb.setTick(200, true);

	auto roundInfo = rb.getRoundInfo(roundId);
	EXPECT_EQ(roundInfo.returnCode, static_cast<uint8>(RBEACON::EReturnCode::SUCCESS));
	EXPECT_TRUE(roundInfo.finalized);
	EXPECT_EQ(static_cast<uint32>(roundInfo.operatorCount), 0U);
	EXPECT_EQ(static_cast<uint32>(roundInfo.revealedCount), 0U);
	EXPECT_EQ(roundInfo.roundRandom, NULL_ID);
	EXPECT_EQ(rb.state()->getCommitCount(), 0ULL);
	EXPECT_EQ(rb.state()->getRevealedOperatorsCount(), 0ULL);
	EXPECT_EQ(static_cast<uint32>(rb.state()->getRevealedCount()), 0U);
	EXPECT_EQ(rb.state()->getPreviousRoundRandom(), NULL_ID);
	EXPECT_EQ(rb.state()->getRoundCount(), 1ULL);

	auto randomOut = rb.getRandom(roundId);
	EXPECT_EQ(randomOut.returnCode, static_cast<uint8>(RBEACON::EReturnCode::INSUFFICIENT_REVEALS));
}

TEST(ContractRBeacon, CommitZeroDeposit)
{
	ContractTestingRBeacon rb;

	rb.setTick(10, true);

	const id operatorAddr = makeUser();
	const id seed = makeSeed();
	const id salt = makeSalt();
	increaseEnergy(operatorAddr, 1);
	const uint64 balanceBefore = getBalance(operatorAddr);

	RBEACON::Commit_input input{};
	input.commitHash = ContractTestingRBeacon::computeCommitHashForTest(seed, salt, 0);
	RBEACON::Commit_output output{};
	rb.invokeUserProcedure(RBEACON_CONTRACT_INDEX, PROCEDURE_INDEX_COMMIT, input, output, operatorAddr, 0);
	EXPECT_EQ(output.returnCode, static_cast<uint8>(RBEACON::EReturnCode::INSUFFICIENT_DEPOSIT));
	EXPECT_EQ(getBalance(operatorAddr), balanceBefore);
	EXPECT_EQ(rb.state()->getCommitCount(), 0ULL);
}

TEST(ContractRBeacon, CommitRoundBoundaryRoundId)
{
	ContractTestingRBeacon rb;

	rb.setTick(99, true);

	const id operatorA = makeUser();
	const id seedA = makeSeed();
	const id saltA = makeSalt();
	increaseEnergy(operatorA, RBEACON_DEPOSIT);
	auto commitOutA = rb.commit(operatorA, seedA, saltA, 0);
	EXPECT_EQ(commitOutA.returnCode, static_cast<uint8>(RBEACON::EReturnCode::INVALID_PHASE));
	EXPECT_EQ(commitOutA.roundId, 0U);
	EXPECT_EQ(rb.state()->getCommitCount(), 0ULL);

	rb.setTick(100, true);

	const id operatorB = makeUser();
	const id seedB = makeSeed();
	const id saltB = makeSalt();
	increaseEnergy(operatorB, RBEACON_DEPOSIT);
	auto commitOutB = rb.commit(operatorB, seedB, saltB, 1);
	EXPECT_EQ(commitOutB.returnCode, static_cast<uint8>(RBEACON::EReturnCode::SUCCESS));
	EXPECT_EQ(commitOutB.roundId, 1U);
	EXPECT_EQ(rb.state()->getCommitCount(), 1ULL);
	EXPECT_TRUE(rb.state()->hasCommit(operatorB));
}

TEST(ContractRBeacon, RevealPhaseBoundaries)
{
	ContractTestingRBeacon rb;

	rb.setTick(10, true);

	const id operatorAddr = makeUser();
	const id seed = makeSeed();
	const id salt = makeSalt();
	increaseEnergy(operatorAddr, RBEACON_DEPOSIT);
	auto commitOut = rb.commit(operatorAddr, seed, salt, 0);
	EXPECT_EQ(commitOut.returnCode, static_cast<uint8>(RBEACON::EReturnCode::SUCCESS));

	rb.setTick(49, true);
	auto revealOutEarly = rb.reveal(operatorAddr, seed, salt);
	EXPECT_EQ(revealOutEarly.returnCode, static_cast<uint8>(RBEACON::EReturnCode::INVALID_PHASE));

	rb.setTick(50, false);
	auto revealOut = rb.reveal(operatorAddr, seed, salt);
	EXPECT_EQ(revealOut.returnCode, static_cast<uint8>(RBEACON::EReturnCode::SUCCESS));
}

TEST(ContractRBeacon, RevealAfterFinalize)
{
	ContractTestingRBeacon rb;

	static constexpr uint32 roundId = 1;
	rb.setTick(100, true);

	const id operatorAddr = makeUser();
	const id seed = makeSeed();
	const id salt = makeSalt();
	increaseEnergy(operatorAddr, RBEACON_DEPOSIT);
	auto commitOut = rb.commit(operatorAddr, seed, salt, roundId);
	EXPECT_EQ(commitOut.returnCode, static_cast<uint8>(RBEACON::EReturnCode::SUCCESS));

	rb.setTick(200, true);
	rb.setTick(250, false);
	auto revealOut = rb.reveal(operatorAddr, seed, salt);
	EXPECT_EQ(revealOut.returnCode, static_cast<uint8>(RBEACON::EReturnCode::NOT_COMMITTED));
	EXPECT_EQ(rb.state()->getCommitCount(), 0ULL);
	EXPECT_EQ(rb.state()->getRevealedOperatorsCount(), 0ULL);
}

TEST(ContractRBeacon, GetRandomWithFeeZeroFee)
{
	ContractTestingRBeacon rb;

	static constexpr uint32 roundId = 1;
	rb.setTick(100, true);

	std::vector<id> operators;
	std::vector<id> seeds;
	std::vector<id> salts;
	for (uint64 i = 0; i < 2; ++i)
	{
		operators.push_back(makeUser());
		seeds.push_back(makeSeed());
		salts.push_back(makeSalt());
		increaseEnergy(operators.back(), RBEACON_DEPOSIT);
		auto commitOut = rb.commit(operators.back(), seeds.back(), salts.back(), roundId);
		EXPECT_EQ(commitOut.returnCode, static_cast<uint8>(RBEACON::EReturnCode::SUCCESS));
	}

	rb.setTick(150, false);
	for (size_t i = 0; i < operators.size(); ++i)
	{
		auto revealOut = rb.reveal(operators[i], seeds[i], salts[i]);
		EXPECT_EQ(revealOut.returnCode, static_cast<uint8>(RBEACON::EReturnCode::SUCCESS));
	}

	rb.setTick(200, true);
	const m256i expectedRandom = xorSeeds(seeds);
	const uint64 treasuryBefore = rb.state()->getTreasury();

	const id caller = makeUser();
	increaseEnergy(caller, 1);
	const uint64 balanceBefore = getBalance(caller);
	auto out = rb.getRandomWithFee(caller, roundId, 0);
	EXPECT_EQ(out.returnCode, static_cast<uint8>(RBEACON::EReturnCode::SUCCESS));
	EXPECT_EQ(out.randomValue, expectedRandom);
	EXPECT_EQ(getBalance(caller), balanceBefore);
	EXPECT_EQ(rb.state()->getTreasury(), treasuryBefore);

	auto roundInfo = rb.getRoundInfo(roundId);
	EXPECT_EQ(roundInfo.rewardPool, 0ULL);
	EXPECT_FALSE(roundInfo.rewardsDistributed);
}

TEST(ContractRBeacon, RewardPerWinnerZero)
{
	ContractTestingRBeacon rb;

	static constexpr uint32 roundId = 1;
	rb.setTick(100, true);

	std::vector<id> operators;
	std::vector<id> seeds;
	std::vector<id> salts;
	for (uint64 i = 0; i < 3; ++i)
	{
		operators.push_back(makeUser());
		seeds.push_back(makeSeed());
		salts.push_back(makeSalt());
		increaseEnergy(operators.back(), RBEACON_DEPOSIT);
		auto commitOut = rb.commit(operators.back(), seeds.back(), salts.back(), roundId);
		EXPECT_EQ(commitOut.returnCode, static_cast<uint8>(RBEACON::EReturnCode::SUCCESS));
	}

	rb.setTick(150, false);
	for (size_t i = 0; i < operators.size(); ++i)
	{
		auto revealOut = rb.reveal(operators[i], seeds[i], salts[i]);
		EXPECT_EQ(revealOut.returnCode, static_cast<uint8>(RBEACON::EReturnCode::SUCCESS));
	}

	rb.setTick(200, true);
	rb.state()->restoreRevealedOperators(operators);

	std::vector<uint64> balancesBefore;
	balancesBefore.reserve(operators.size());
	for (const auto& op : operators)
	{
		balancesBefore.push_back(getBalance(op));
	}

	const id caller = makeUser();
	static constexpr uint64 fee = 3;
	increaseEnergy(caller, fee);
	auto out = rb.getRandomWithFee(caller, roundId, fee);
	EXPECT_EQ(out.returnCode, static_cast<uint8>(RBEACON::EReturnCode::SUCCESS));

	for (size_t i = 0; i < operators.size(); ++i)
	{
		EXPECT_EQ(getBalance(operators[i]), balancesBefore[i]);
	}

	auto roundInfo = rb.getRoundInfo(roundId);
	EXPECT_EQ(roundInfo.rewardPool, 2ULL);
	EXPECT_TRUE(roundInfo.rewardsDistributed);
}

TEST(ContractRBeacon, TreasuryFeeSplit)
{
	ContractTestingRBeacon rb;

	static constexpr uint32 roundId = 1;
	rb.setTick(100, true);

	std::vector<id> operators;
	std::vector<id> seeds;
	std::vector<id> salts;
	for (uint64 i = 0; i < 2; ++i)
	{
		operators.push_back(makeUser());
		seeds.push_back(makeSeed());
		salts.push_back(makeSalt());
		increaseEnergy(operators.back(), RBEACON_DEPOSIT);
		auto commitOut = rb.commit(operators.back(), seeds.back(), salts.back(), roundId);
		EXPECT_EQ(commitOut.returnCode, static_cast<uint8>(RBEACON::EReturnCode::SUCCESS));
	}

	rb.setTick(150, false);
	for (size_t i = 0; i < operators.size(); ++i)
	{
		auto revealOut = rb.reveal(operators[i], seeds[i], salts[i]);
		EXPECT_EQ(revealOut.returnCode, static_cast<uint8>(RBEACON::EReturnCode::SUCCESS));
	}

	rb.setTick(200, true);
	rb.state()->restoreRevealedOperators(operators);

	static constexpr uint64 fee = 1000;
	const uint64 treasuryBefore = rb.state()->getTreasury();
	const id caller = makeUser();
	increaseEnergy(caller, fee);
	auto out = rb.getRandomWithFee(caller, roundId, fee);
	EXPECT_EQ(out.returnCode, static_cast<uint8>(RBEACON::EReturnCode::SUCCESS));

	const uint64 expectedTreasury = treasuryBefore + (fee - (fee * RBEACON_REWARD_PERCENT) / 100ULL);
	EXPECT_EQ(rb.state()->getTreasury(), expectedTreasury);
}

TEST(ContractRBeacon, RoundHistoryMultipleRounds)
{
	ContractTestingRBeacon rb;

	static constexpr uint32 round1 = 1;
	rb.setTick(100, true);

	const id operatorA = makeUser();
	const id seedA = makeSeed();
	const id saltA = makeSalt();
	const id operatorB = makeUser();
	const id seedB = makeSeed();
	const id saltB = makeSalt();
	increaseEnergy(operatorA, RBEACON_DEPOSIT);
	increaseEnergy(operatorB, RBEACON_DEPOSIT);
	EXPECT_EQ(rb.commit(operatorA, seedA, saltA, round1).returnCode, static_cast<uint8>(RBEACON::EReturnCode::SUCCESS));
	EXPECT_EQ(rb.commit(operatorB, seedB, saltB, round1).returnCode, static_cast<uint8>(RBEACON::EReturnCode::SUCCESS));

	rb.setTick(150, false);
	EXPECT_EQ(rb.reveal(operatorA, seedA, saltA).returnCode, static_cast<uint8>(RBEACON::EReturnCode::SUCCESS));
	EXPECT_EQ(rb.reveal(operatorB, seedB, saltB).returnCode, static_cast<uint8>(RBEACON::EReturnCode::SUCCESS));

	rb.setTick(200, true);

	static constexpr uint32 round2 = 2;
	rb.setTick(210, false);
	const id operatorC = makeUser();
	const id seedC = makeSeed();
	const id saltC = makeSalt();
	const id operatorD = makeUser();
	const id seedD = makeSeed();
	const id saltD = makeSalt();
	increaseEnergy(operatorC, RBEACON_DEPOSIT);
	increaseEnergy(operatorD, RBEACON_DEPOSIT);
	EXPECT_EQ(rb.commit(operatorC, seedC, saltC, round2).returnCode, static_cast<uint8>(RBEACON::EReturnCode::SUCCESS));
	EXPECT_EQ(rb.commit(operatorD, seedD, saltD, round2).returnCode, static_cast<uint8>(RBEACON::EReturnCode::SUCCESS));

	rb.setTick(250, false);
	EXPECT_EQ(rb.reveal(operatorC, seedC, saltC).returnCode, static_cast<uint8>(RBEACON::EReturnCode::SUCCESS));
	EXPECT_EQ(rb.reveal(operatorD, seedD, saltD).returnCode, static_cast<uint8>(RBEACON::EReturnCode::SUCCESS));

	rb.setTick(300, true);

	auto round1Info = rb.getRoundInfo(round1);
	EXPECT_EQ(round1Info.returnCode, static_cast<uint8>(RBEACON::EReturnCode::SUCCESS));
	EXPECT_TRUE(round1Info.finalized);

	auto round2Info = rb.getRoundInfo(round2);
	EXPECT_EQ(round2Info.returnCode, static_cast<uint8>(RBEACON::EReturnCode::SUCCESS));
	EXPECT_TRUE(round2Info.finalized);
}

TEST(ContractRBeacon, RewardsDistributedOnceWithFewReveals)
{
	ContractTestingRBeacon rb;

	static constexpr uint32 roundId = 1;
	rb.setTick(100, true);

	std::vector<id> operators;
	std::vector<id> seeds;
	std::vector<id> salts;
	for (uint64 i = 0; i < 3; ++i)
	{
		operators.push_back(makeUser());
		seeds.push_back(makeSeed());
		salts.push_back(makeSalt());
		increaseEnergy(operators.back(), RBEACON_DEPOSIT);
		auto commitOut = rb.commit(operators.back(), seeds.back(), salts.back(), roundId);
		EXPECT_EQ(commitOut.returnCode, static_cast<uint8>(RBEACON::EReturnCode::SUCCESS));
	}

	rb.setTick(150, false);
	for (size_t i = 0; i < operators.size(); ++i)
	{
		auto revealOut = rb.reveal(operators[i], seeds[i], salts[i]);
		EXPECT_EQ(revealOut.returnCode, static_cast<uint8>(RBEACON::EReturnCode::SUCCESS));
	}

	rb.setTick(200, true);
	const m256i expectedRandom = xorSeeds(seeds);

	rb.state()->restoreRevealedOperators(operators);

	static constexpr uint64 fee = 90000;
	static constexpr uint64 rewardPool = (fee * RBEACON_REWARD_PERCENT) / 100ULL;
	const uint64 rewardPerWinner = rewardPool / operators.size();

	std::vector<uint64> balancesBefore;
	balancesBefore.reserve(operators.size());
	for (const auto& op : operators)
	{
		balancesBefore.push_back(getBalance(op));
	}

	const id caller = makeUser();
	increaseEnergy(caller, fee);
	auto out = rb.getRandomWithFee(caller, roundId, fee);
	EXPECT_EQ(out.returnCode, static_cast<uint8>(RBEACON::EReturnCode::SUCCESS));
	EXPECT_EQ(out.randomValue, expectedRandom);

	for (size_t i = 0; i < operators.size(); ++i)
	{
		EXPECT_EQ(getBalance(operators[i]), balancesBefore[i] + rewardPerWinner);
	}

	static constexpr uint64 secondFee = 50000;
	increaseEnergy(caller, secondFee);
	auto secondOut = rb.getRandomWithFee(caller, roundId, secondFee);
	EXPECT_EQ(secondOut.returnCode, static_cast<uint8>(RBEACON::EReturnCode::SUCCESS));
	for (size_t i = 0; i < operators.size(); ++i)
	{
		EXPECT_EQ(getBalance(operators[i]), balancesBefore[i] + rewardPerWinner);
	}

	auto roundInfo = rb.getRoundInfo(roundId);
	static constexpr uint64 totalRewardPool = rewardPool + (secondFee * RBEACON_REWARD_PERCENT) / 100ULL;
	EXPECT_EQ(roundInfo.rewardPool, totalRewardPool);
	EXPECT_TRUE(roundInfo.rewardsDistributed);
}

TEST(ContractRBeacon, TopKRewardsDistribution)
{
	ContractTestingRBeacon rb;

	static constexpr uint32 roundId = 1;
	rb.setTick(100, true);

	std::vector<id> operators;
	std::vector<id> seeds;
	std::vector<id> salts;
	for (uint64 i = 0; i < RBEACON_K_WINNERS + 1; ++i)
	{
		operators.push_back(makeUser());
		seeds.push_back(makeSeed());
		salts.push_back(makeSalt());
		increaseEnergy(operators.back(), RBEACON_DEPOSIT);
		auto commitOut = rb.commit(operators.back(), seeds.back(), salts.back(), roundId);
		EXPECT_EQ(commitOut.returnCode, static_cast<uint8>(RBEACON::EReturnCode::SUCCESS));
	}

	rb.setTick(150, false);
	for (size_t i = 0; i < operators.size(); ++i)
	{
		auto revealOut = rb.reveal(operators[i], seeds[i], salts[i]);
		EXPECT_EQ(revealOut.returnCode, static_cast<uint8>(RBEACON::EReturnCode::SUCCESS));
	}

	rb.setTick(200, true);

	auto roundInfo = rb.getRoundInfo(roundId);
	EXPECT_EQ(roundInfo.returnCode, static_cast<uint8>(RBEACON::EReturnCode::SUCCESS));
	const m256i expectedRandom = xorSeeds(seeds);
	EXPECT_EQ(roundInfo.roundRandom, expectedRandom);

	// Rehydrate revealedOperators for reward distribution logic.
	rb.state()->restoreRevealedOperators(operators);

	struct ScoredOperator
	{
		id addr;
		m256i score;
	};
	std::vector<ScoredOperator> scored;
	scored.reserve(operators.size());
	for (const auto& op : operators)
	{
		scored.push_back({op, computeWinnerScore(expectedRandom, op)});
	}

	std::sort(scored.begin(), scored.end(),
	          [](const ScoredOperator& a, const ScoredOperator& b) { return RBeaconChecker::isScoreLess(a.score, b.score); });

	std::vector<id> winners;
	winners.reserve(RBEACON_K_WINNERS);
	for (uint32 i = 0; i < RBEACON_K_WINNERS; ++i)
	{
		winners.push_back(scored[i].addr);
	}
	const id loser = scored.back().addr;

	static constexpr uint64 fee = 200000;
	static constexpr uint64 rewardPool = (fee * RBEACON_REWARD_PERCENT) / 100ULL;
	static constexpr uint64 rewardPerWinner = rewardPool / RBEACON_K_WINNERS;

	std::vector<uint64> balancesBefore;
	balancesBefore.reserve(winners.size());
	for (const auto& winner : winners)
	{
		balancesBefore.push_back(getBalance(winner));
	}
	const uint64 loserBalanceBefore = getBalance(loser);

	const id caller = makeUser();
	increaseEnergy(caller, fee);
	auto out = rb.getRandomWithFee(caller, roundId, fee);
	EXPECT_EQ(out.returnCode, static_cast<uint8>(RBEACON::EReturnCode::SUCCESS));

	for (size_t i = 0; i < winners.size(); ++i)
	{
		EXPECT_EQ(getBalance(winners[i]), balancesBefore[i] + rewardPerWinner);
	}
	EXPECT_EQ(getBalance(loser), loserBalanceBefore);
}
