/**
 * @file RandBeacon.h
 * @brief Random Beacon contract: decentralized randomness beacon with commit-reveal scheme.
 *
 * This contract implements a multi-participant commit-reveal random number generator:
 *  - Operators commit hash(seed || salt || round_id) during commit phase (ticks 0-49)
 *  - Operators reveal seed+salt during reveal phase (ticks 50-99)
 *  - Random is generated as XOR of all revealed seeds at round boundary
 *  - K winners selected deterministically via hash(random || address) for rewards
 *  - External SCs can request random via GetRandomWithFee, triggering reward distribution
 */

using namespace QPI;

// Round configuration
constexpr uint32 RBEACON_ROUND_LENGTH = 100;       // Round length in ticks
constexpr uint32 RBEACON_COMMIT_END = 50;          // Commit phase: ticks 0 to COMMIT_END-1
constexpr uint64 RBEACON_DEPOSIT = 10000;          // Operator deposit (refundable on reveal)

// Capacity limits (must be powers of 2)
constexpr uint32 RBEACON_MAX_OPERATORS = 1024;     // Max operators per round
constexpr uint32 RBEACON_MAX_ROUNDS = 256;         // Max rounds in history

// Reward configuration
constexpr uint32 RBEACON_K_WINNERS = 20;           // Number of winners for rewards
constexpr uint32 RBEACON_REWARD_PERCENT = 80;      // % of fees to operators
constexpr uint32 RBEACON_TREASURY_PERCENT = 20;    // % of fees to treasury
constexpr uint16 RBEACON_MIN_REVEALS = 2;          // Minimum reveals required to use round random
constexpr uint32 RBEACON_TOPK_CAPACITY = 32;       // Must be power of 2, >= RBEACON_K_WINNERS

// Placeholder for future extensions
struct RBEACON2
{
};

/**
 * @brief Main contract implementing the random beacon mechanics.
 *
 * Lifecycle per round:
 *  1. Ticks 0-49: COMMIT phase - operators submit commitHash + deposit
 *  2. Ticks 50-99: REVEAL phase - operators reveal seed+salt, get deposit back
 *  3. Tick 0 of next round: Finalize - generate random, forfeit unrevealed deposits
 *  4. External SC calls GetRandomWithFee - triggers reward distribution to K winners
 */
struct RBEACON : public ContractBase
{
public:
    /**
     * @brief Return codes for procedures
     */
    enum class EReturnCode : uint8
    {
        SUCCESS = 0,
        INVALID_PHASE = 1,
        INSUFFICIENT_DEPOSIT = 2,
        ALREADY_COMMITTED = 3,
        NOT_COMMITTED = 4,
        ALREADY_REVEALED = 5,
        HASH_MISMATCH = 6,
        ROUND_NOT_FINALIZED = 7,
        ROUND_NOT_FOUND = 8,
        MAX_OPERATORS_REACHED = 9,
        INSUFFICIENT_REVEALS = 10
    };

    /**
     * @brief Operator commitment record
     */
    struct OperatorCommit
    {
        m256i commitHash;        // hash(seed || salt || round_id)
        m256i seed;              // Revealed seed (zero until revealed)
        uint64 deposit;       // Locked deposit amount
        bit revealed;         // True if successfully revealed
    };

    struct CommitHashInput
    {
        m256i seed;
        m256i salt;
        m256i round;
    };

    struct WinnerHashInput
    {
        m256i random;
        id operatorAddr;
    };

    /**
     * @brief Round data stored in history
     */
    struct RoundData
    {
        m256i roundRandom;           // Generated random for this round
        uint64 rewardPool;        // Accumulated fees for rewards
        uint32 roundId;           // Round identifier
        uint16 operatorCount;     // Total operators who committed
        uint16 revealedCount;     // Operators who successfully revealed
        bit finalized;            // Round has been finalized
        bit rewardsDistributed;   // Rewards have been paid out
    };

    //---- Input/Output structures ----

    struct Commit_input
    {
        m256i commitHash;  // K12(CommitHashInput{seed, salt, roundId})
    };

    struct Commit_output
    {
        uint8 returnCode;
        uint32 roundId;
    };

    struct Commit_locals
    {
        OperatorCommit newCommit;
    };

    struct Reveal_input
    {
        m256i seed;
        m256i salt;
    };

    struct Reveal_output
    {
        uint8 returnCode;
    };

    struct Reveal_locals
    {
        OperatorCommit record;
        m256i computedHash;
        CommitHashInput hashInput;
        bit found;
    };

    struct GetRandomWithFee_input
    {
        uint32 roundId;
    };

    struct GetRandomWithFee_output
    {
        m256i randomValue;
        uint8 returnCode;
    };

    struct GetRandomWithFee_locals
    {
        RoundData roundData;
        uint64 operatorReward;
        uint64 treasuryAmount;
        uint64 rewardPerWinner;
        uint32 winnersCount;
        uint32 topCount;
        uint32 worstIndex;
        sint64 i;
        sint64 j;
        id operatorAddr;
        m256i currentScore;
        WinnerHashInput winnerHashInput;
        Array<id, RBEACON_TOPK_CAPACITY> winnerAddrs;
        Array<m256i, RBEACON_TOPK_CAPACITY> winnerScores;
        bit found;
    };

    struct GetRandom_input
    {
        uint32 roundId;
    };

    struct GetRandom_output
    {
        m256i randomValue;
        uint8 returnCode;
    };

    struct GetRandom_locals
    {
        RoundData roundData;
        bit found;
    };

    struct GetRoundInfo_input
    {
        uint32 roundId;
    };

    struct GetRoundInfo_output
    {
        uint32 roundId;
        m256i roundRandom;
        uint64 rewardPool;
        uint16 operatorCount;
        uint16 revealedCount;
        bit finalized;
        bit rewardsDistributed;
        uint8 returnCode;
    };

    struct GetRoundInfo_locals
    {
        RoundData roundData;
        bit found;
    };

    struct GetCurrentRound_input
    {
    };

    struct GetCurrentRound_output
    {
        uint32 roundId;
        uint32 roundTick;
        bit isCommitPhase;
        bit isRevealPhase;
        uint16 operatorCount;
        uint16 revealedCount;
    };

    struct BEGIN_TICK_locals
    {
        uint32 newRoundId;
        sint64 i;
        OperatorCommit record;
        m256i combinedSeeds;
        RoundData roundData;
    };

    //---- Public interface ----

    REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
    {
        REGISTER_USER_FUNCTION(GetRandom, 1);
        REGISTER_USER_FUNCTION(GetRoundInfo, 2);
        REGISTER_USER_FUNCTION(GetCurrentRound, 3);
        REGISTER_USER_PROCEDURE(Commit, 1);
        REGISTER_USER_PROCEDURE(Reveal, 2);
        REGISTER_USER_PROCEDURE(GetRandomWithFee, 3);
    }

    INITIALIZE()
    {
        state.currentRoundId = 0;
        state.treasury = 0;
        state.previousRoundRandom = NULL_ID;
        state.revealedCount = 0;
    }

    /**
     * @brief BEGIN_TICK handles round transitions and finalization
     */
    BEGIN_TICK_WITH_LOCALS()
    {
        locals.newRoundId = div<uint32>(qpi.tick(), RBEACON_ROUND_LENGTH);

        // Check if we're entering a new round
        if (locals.newRoundId > state.currentRoundId && state.currentRoundId > 0)
        {
            // Finalize the previous round

            // Step 1: Generate random from XOR of all revealed seeds
            locals.combinedSeeds = NULL_ID;

            locals.i = state.commits.nextElementIndex(NULL_INDEX);
            while (locals.i != NULL_INDEX)
            {
                locals.record = state.commits.value(locals.i);
                if (locals.record.revealed)
                {
                    // XOR the seed into combined
                    locals.combinedSeeds.u64._0 ^= locals.record.seed.u64._0;
                    locals.combinedSeeds.u64._1 ^= locals.record.seed.u64._1;
                    locals.combinedSeeds.u64._2 ^= locals.record.seed.u64._2;
                    locals.combinedSeeds.u64._3 ^= locals.record.seed.u64._3;
                }
                else
                {
                    // Forfeit unrevealed deposit to treasury
                    state.treasury += locals.record.deposit;
                }
                locals.i = state.commits.nextElementIndex(locals.i);
            }

            // If no reveals, keep combinedSeeds at NULL_ID; round will be unusable below

            // Step 2: Store round data in history
            locals.roundData.roundId = state.currentRoundId;
            locals.roundData.roundRandom = locals.combinedSeeds;
            locals.roundData.operatorCount = static_cast<uint16>(state.commits.population());
            locals.roundData.revealedCount = state.revealedCount;
            locals.roundData.rewardPool = 0;
            locals.roundData.finalized = true;
            locals.roundData.rewardsDistributed = false;

            state.rounds.set(state.currentRoundId, locals.roundData);

            // Update previous random for next fallback
            if (state.revealedCount >= RBEACON_MIN_REVEALS)
            {
                state.previousRoundRandom = locals.combinedSeeds;
            }

            // Step 3: Clear state for new round
            state.commits.reset();
            state.revealedOperators.reset();
            state.revealedCount = 0;
        }

        // Update current round ID
        state.currentRoundId = locals.newRoundId;
    }

    /**
     * @brief Commit: Operator submits hash(seed || salt || roundId) with deposit
     */
    PUBLIC_PROCEDURE_WITH_LOCALS(Commit)
    {
        output.returnCode = static_cast<uint8>(EReturnCode::SUCCESS);
        output.roundId = state.currentRoundId;

        // Check we're in commit phase (ticks 0 to COMMIT_END-1)
        if (mod<uint32>(qpi.tick(), RBEACON_ROUND_LENGTH) >= RBEACON_COMMIT_END)
        {
            output.returnCode = static_cast<uint8>(EReturnCode::INVALID_PHASE);
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            return;
        }

        // Check sufficient deposit
        if (qpi.invocationReward() < RBEACON_DEPOSIT)
        {
            output.returnCode = static_cast<uint8>(EReturnCode::INSUFFICIENT_DEPOSIT);
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            return;
        }

        // Check not already committed
        if (state.commits.contains(qpi.invocator()))
        {
            output.returnCode = static_cast<uint8>(EReturnCode::ALREADY_COMMITTED);
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        // Check capacity
        if (state.commits.population() >= RBEACON_MAX_OPERATORS)
        {
            output.returnCode = static_cast<uint8>(EReturnCode::MAX_OPERATORS_REACHED);
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            return;
        }

        // Store commitment
        locals.newCommit.commitHash = input.commitHash;
        locals.newCommit.seed = NULL_ID;
        locals.newCommit.deposit = qpi.invocationReward();
        locals.newCommit.revealed = false;

        state.commits.set(qpi.invocator(), locals.newCommit);
    }

    /**
     * @brief Reveal: Operator reveals seed+salt, contract verifies hash matches
     */
    PUBLIC_PROCEDURE_WITH_LOCALS(Reveal)
    {
        output.returnCode = static_cast<uint8>(EReturnCode::SUCCESS);

        // Check we're in reveal phase (ticks COMMIT_END to ROUND_LENGTH-1)
        if (mod<uint32>(qpi.tick(), RBEACON_ROUND_LENGTH) < RBEACON_COMMIT_END)
        {
            output.returnCode = static_cast<uint8>(EReturnCode::INVALID_PHASE);
            return;
        }

        // Check operator committed
        locals.found = state.commits.get(qpi.invocator(), locals.record);
        if (!locals.found)
        {
            output.returnCode = static_cast<uint8>(EReturnCode::NOT_COMMITTED);
            return;
        }

        // Check not already revealed
        if (locals.record.revealed)
        {
            output.returnCode = static_cast<uint8>(EReturnCode::ALREADY_REVEALED);
            return;
        }

        // Verify hash: K12(seed || salt || roundId) == commitHash
        locals.hashInput.seed = input.seed;
        locals.hashInput.salt = input.salt;
        locals.hashInput.round = NULL_ID;
        locals.hashInput.round.u64._0 = state.currentRoundId;
        locals.computedHash = qpi.K12(locals.hashInput);

        if (locals.computedHash != locals.record.commitHash)
        {
            output.returnCode = static_cast<uint8>(EReturnCode::HASH_MISMATCH);
            state.commits.removeByKey(qpi.invocator());
            return;
        }

        // Update record
        locals.record.seed = input.seed;
        locals.record.revealed = true;
        state.commits.replace(qpi.invocator(), locals.record);

        // Add to revealed operators list
        state.revealedOperators.add(qpi.invocator());
        state.revealedCount++;

        // Return deposit
        qpi.transfer(qpi.invocator(), locals.record.deposit);
    }

    /**
     * @brief GetRandomWithFee: External SC requests random and pays fee, triggering rewards
     */
    PUBLIC_PROCEDURE_WITH_LOCALS(GetRandomWithFee)
    {
        output.returnCode = static_cast<uint8>(EReturnCode::SUCCESS);

        // Find round in history
        locals.found = state.rounds.get(input.roundId, locals.roundData);
        if (!locals.found)
        {
            output.returnCode = static_cast<uint8>(EReturnCode::ROUND_NOT_FOUND);
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            return;
        }

        // Check round is finalized
        if (!locals.roundData.finalized)
        {
            output.returnCode = static_cast<uint8>(EReturnCode::ROUND_NOT_FINALIZED);
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            return;
        }

        if (locals.roundData.revealedCount < RBEACON_MIN_REVEALS)
        {
            output.returnCode = static_cast<uint8>(EReturnCode::INSUFFICIENT_REVEALS);
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            return;
        }

        output.randomValue = locals.roundData.roundRandom;

        // Process fee if any
        if (qpi.invocationReward() > 0)
        {
            // Split fee: 80% to reward pool, 20% to treasury
            locals.operatorReward = div<uint64>(smul(qpi.invocationReward() , static_cast<sint64>(RBEACON_REWARD_PERCENT)), 100ULL);
            locals.treasuryAmount = qpi.invocationReward() - locals.operatorReward;

            state.treasury += locals.treasuryAmount;
            locals.roundData.rewardPool += locals.operatorReward;

            // Distribute rewards if not done yet and we have revealed operators
            if (!locals.roundData.rewardsDistributed && locals.roundData.revealedCount > 0)
            {
                // Calculate number of winners: min(K, revealedCount)
                locals.winnersCount = locals.roundData.revealedCount;
                if (locals.winnersCount > RBEACON_K_WINNERS)
                {
                    locals.winnersCount = RBEACON_K_WINNERS;
                }

                // Select Top-K winners by score = K12(roundRandom || operatorAddr)
                locals.i = state.revealedOperators.nextElementIndex(NULL_INDEX);
                while (locals.i != NULL_INDEX)
                {
                    locals.operatorAddr = state.revealedOperators.key(locals.i);
                    locals.winnerHashInput.random = locals.roundData.roundRandom;
                    locals.winnerHashInput.operatorAddr = locals.operatorAddr;
                    locals.currentScore = qpi.K12(locals.winnerHashInput);

                    if (locals.topCount < locals.winnersCount)
                    {
                        locals.winnerAddrs.set(locals.topCount, locals.operatorAddr);
                        locals.winnerScores.set(locals.topCount, locals.currentScore);
                        locals.topCount++;
                    }
                    else
                    {
                        // Find worst (highest) score in current Top-K
                        locals.worstIndex = 0;
                        locals.j = 1;
                        while (locals.j < locals.topCount)
                        {
                            if (isScoreLess(locals.winnerScores.get(locals.worstIndex), locals.winnerScores.get(locals.j)))
                            {
                                locals.worstIndex = static_cast<uint32>(locals.j);
                            }
                            locals.j++;
                        }

                        // Replace worst if current score is better (lower)
                        if (isScoreLess(locals.currentScore, locals.winnerScores.get(locals.worstIndex)))
                        {
                            locals.winnerAddrs.set(locals.worstIndex, locals.operatorAddr);
                            locals.winnerScores.set(locals.worstIndex, locals.currentScore);
                        }
                    }

                    locals.i = state.revealedOperators.nextElementIndex(locals.i);
                }

                if (locals.topCount > 0)
                {
                    locals.rewardPerWinner = div<uint64>(locals.roundData.rewardPool, (uint64)locals.topCount);
                    if (locals.rewardPerWinner > 0)
                    {
                        locals.j = 0;
                        while (locals.j < locals.topCount)
                        {
                            locals.operatorAddr = locals.winnerAddrs.get(static_cast<uint32>(locals.j));
                            qpi.transfer(locals.operatorAddr, locals.rewardPerWinner);
                            locals.j++;
                        }
                    }
                }

                locals.roundData.rewardsDistributed = true;
            }

            // Update round data in history
            state.rounds.replace(input.roundId, locals.roundData);
        }
    }

    /**
     * @brief GetRandom: Query random for a round (no fee, no rewards)
     */
    PUBLIC_FUNCTION_WITH_LOCALS(GetRandom)
    {
        output.returnCode = static_cast<uint8>(EReturnCode::SUCCESS);

        // Find round in history
        locals.found = state.rounds.get(input.roundId, locals.roundData);
        if (!locals.found)
        {
            output.returnCode = static_cast<uint8>(EReturnCode::ROUND_NOT_FOUND);
            return;
        }

        if (locals.roundData.revealedCount < RBEACON_MIN_REVEALS)
        {
            output.returnCode = static_cast<uint8>(EReturnCode::INSUFFICIENT_REVEALS);
            return;
        }

        output.randomValue = locals.roundData.roundRandom;
    }

    /**
     * @brief GetRoundInfo: Query full info about a specific round
     */
    PUBLIC_FUNCTION_WITH_LOCALS(GetRoundInfo)
    {
        output.returnCode = static_cast<uint8>(EReturnCode::SUCCESS);

        locals.found = state.rounds.get(input.roundId, locals.roundData);
        if (!locals.found)
        {
            output.returnCode = static_cast<uint8>(EReturnCode::ROUND_NOT_FOUND);
            return;
        }

        output.roundId = locals.roundData.roundId;
        output.roundRandom = locals.roundData.roundRandom;
        output.rewardPool = locals.roundData.rewardPool;
        output.operatorCount = locals.roundData.operatorCount;
        output.revealedCount = locals.roundData.revealedCount;
        output.finalized = locals.roundData.finalized;
        output.rewardsDistributed = locals.roundData.rewardsDistributed;
    }

    /**
     * @brief GetCurrentRound: Query current round status
     */
    PUBLIC_FUNCTION(GetCurrentRound)
    {
        output.roundId = state.currentRoundId;
        output.roundTick = mod<uint32>(qpi.tick(), RBEACON_ROUND_LENGTH);
        output.isCommitPhase = (output.roundTick < RBEACON_COMMIT_END);
        output.isRevealPhase = (output.roundTick >= RBEACON_COMMIT_END);
        output.operatorCount = static_cast<uint16>(state.commits.population());
        output.revealedCount = state.revealedCount;
    }

protected:
    // Current round data
    HashMap<id, OperatorCommit, RBEACON_MAX_OPERATORS> commits;
    HashSet<id, RBEACON_MAX_OPERATORS> revealedOperators;
    uint32 currentRoundId;
    uint16 revealedCount;

    // Historical data - directly maps roundId to RoundData
    HashMap<uint32, RoundData, RBEACON_MAX_ROUNDS> rounds;

    // Finances
    uint64 treasury;

    // For fallback random generation
    m256i previousRoundRandom;

private:
    static inline bit isScoreLess(const m256i& a, const m256i& b)
    {
        if (a.u64._3 != b.u64._3) return a.u64._3 < b.u64._3;
        if (a.u64._2 != b.u64._2) return a.u64._2 < b.u64._2;
        if (a.u64._1 != b.u64._1) return a.u64._1 < b.u64._1;
        return a.u64._0 < b.u64._0;
    }
};
