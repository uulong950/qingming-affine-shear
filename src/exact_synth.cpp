#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <utility>
#include <vector>

namespace qas {

constexpr int N = 5;
constexpr uint8_t VMASK = 0x1f;

struct Mat5
{
    // Column representation: col[j] = M * e_j, encoded as a 5-bit vector.
    std::array<uint8_t, N> col{};
};

static Mat5 zero5()
{
    return Mat5{};
}

static Mat5 identity5()
{
    Mat5 m;
    for(int j = 0; j < N; ++j)
        m.col[j] = static_cast<uint8_t>(1u << j);
    return m;
}

static uint32_t encode(const Mat5& m)
{
    uint32_t code = 0;
    for(int j = 0; j < N; ++j)
        code |= (static_cast<uint32_t>(m.col[j] & VMASK) << (5 * j));
    return code;
}

static Mat5 decode(uint32_t code)
{
    Mat5 m;
    for(int j = 0; j < N; ++j)
        m.col[j] = static_cast<uint8_t>((code >> (5 * j)) & VMASK);
    return m;
}

static uint8_t apply(const Mat5& m, uint8_t x)
{
    uint8_t y = 0;
    for(int j = 0; j < N; ++j)
        if((x >> j) & 1u)
            y ^= m.col[j];
    return static_cast<uint8_t>(y & VMASK);
}

static uint8_t apply_code(uint32_t code, uint8_t x)
{
    uint8_t y = 0;
    for(int j = 0; j < N; ++j)
        if((x >> j) & 1u)
            y ^= static_cast<uint8_t>((code >> (5 * j)) & VMASK);
    return static_cast<uint8_t>(y & VMASK);
}

static Mat5 add(const Mat5& a, const Mat5& b)
{
    Mat5 c;
    for(int j = 0; j < N; ++j)
        c.col[j] = static_cast<uint8_t>(a.col[j] ^ b.col[j]);
    return c;
}

static Mat5 compose(const Mat5& a, const Mat5& b)
{
    // a * b
    Mat5 c;
    for(int j = 0; j < N; ++j)
        c.col[j] = apply(a, b.col[j]);
    return c;
}

static uint32_t compose_code(uint32_t a_code, const Mat5& b)
{
    uint32_t out = 0;
    for(int j = 0; j < N; ++j)
    {
        const uint8_t v = apply_code(a_code, b.col[j]);
        out |= (static_cast<uint32_t>(v) << (5 * j));
    }
    return out;
}

template <typename T>
static int rank_vectors(std::vector<T> vecs, int max_bits)
{
    std::vector<T> basis(max_bits, 0);
    int r = 0;

    for(T x : vecs)
    {
        T y = x;
        for(int b = max_bits - 1; b >= 0; --b)
        {
            if(((y >> b) & static_cast<T>(1)) == 0)
                continue;

            if(basis[b] != 0)
            {
                y ^= basis[b];
            }
            else
            {
                basis[b] = y;
                ++r;
                break;
            }
        }
    }
    return r;
}

static int rank5(const Mat5& m)
{
    std::vector<uint16_t> v;
    v.reserve(5);
    for(auto c : m.col)
        v.push_back(c);
    return rank_vectors<uint16_t>(std::move(v), 5);
}

static int rank5_code(uint32_t code)
{
    std::array<uint8_t, 5> basis{};
    int r = 0;

    for(int j = 0; j < 5; ++j)
    {
        uint8_t y = static_cast<uint8_t>((code >> (5 * j)) & 31u);
        for(int b = 4; b >= 0; --b)
        {
            if(((y >> b) & 1u) == 0)
                continue;
            if(basis[b])
                y ^= basis[b];
            else
            {
                basis[b] = y;
                ++r;
                break;
            }
        }
    }
    return r;
}

static int rank_hcat(const Mat5& a, const Mat5& b)
{
    std::vector<uint16_t> v;
    v.reserve(10);
    for(auto c : a.col) v.push_back(c);
    for(auto c : b.col) v.push_back(c);
    return rank_vectors<uint16_t>(std::move(v), 5);
}

static int rank_vcat(const Mat5& top, const Mat5& bottom)
{
    std::vector<uint16_t> v;
    v.reserve(5);
    for(int j = 0; j < 5; ++j)
    {
        const uint16_t x =
            static_cast<uint16_t>(top.col[j]) |
            (static_cast<uint16_t>(bottom.col[j]) << 5);
        v.push_back(x);
    }
    return rank_vectors<uint16_t>(std::move(v), 10);
}

static int popcount32(uint32_t x)
{
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_popcount(x);
#else
    int c = 0;
    while(x) { x &= (x - 1); ++c; }
    return c;
#endif
}

static int popcount8(uint8_t x)
{
    return popcount32(x);
}

static void print_matrix(const Mat5& m, const std::string& name)
{
    std::cout << name << " (rows):\n";
    for(int i = 0; i < 5; ++i)
    {
        std::cout << "  [ ";
        for(int j = 0; j < 5; ++j)
            std::cout << ((m.col[j] >> i) & 1u) << (j == 4 ? " " : " ");
        std::cout << "]\n";
    }
}

static Mat5 shift_left1_mod32()
{
    Mat5 m;
    for(int j = 0; j < 5; ++j)
        m.col[j] = static_cast<uint8_t>(((1u << j) << 1) & 31u);
    return m;
}

static uint8_t ck_mix_scalar(uint8_t r)
{
    return static_cast<uint8_t>((r ^ (r >> 2) ^ (r << 3)) & 0x1f);
}

static Mat5 ck_patch_matrix()
{
    Mat5 p;
    for(int j = 0; j < 5; ++j)
        p.col[j] = ck_mix_scalar(static_cast<uint8_t>(1u << j));
    return p;
}

// -----------------------------------------------------------------------------
// Grassmann/subspace verifier for GF(2)^5
// -----------------------------------------------------------------------------

struct Grassmann5
{
    // A subspace is a 32-bit membership mask over the 32 vectors in GF(2)^5.
    std::array<std::vector<uint32_t>, 6> by_dim;

    static uint32_t span_add(uint32_t subspace, uint8_t v)
    {
        uint32_t out = subspace;
        for(int x = 0; x < 32; ++x)
            if((subspace >> x) & 1u)
                out |= (1u << (x ^ v));
        return out;
    }

    Grassmann5()
    {
        std::unordered_set<uint32_t> seen;
        std::queue<uint32_t> q;

        const uint32_t zero_space = 1u << 0; // {0}
        seen.insert(zero_space);
        q.push(zero_space);

        while(!q.empty())
        {
            const uint32_t s = q.front();
            q.pop();

            const int size = popcount32(s);
            int dim = 0;
            int tmp = size;
            while(tmp > 1) { tmp >>= 1; ++dim; }
            by_dim[dim].push_back(s);

            for(uint8_t v = 1; v < 32; ++v)
            {
                if((s >> v) & 1u)
                    continue;
                const uint32_t t = span_add(s, v);
                if(seen.insert(t).second)
                    q.push(t);
            }
        }

        for(auto& vec : by_dim)
            std::sort(vec.begin(), vec.end());
    }

    uint32_t kernel_mask(const Mat5& m) const
    {
        uint32_t mask = 0;
        for(uint8_t x = 0; x < 32; ++x)
            if(apply(m, x) == 0)
                mask |= (1u << x);
        return mask;
    }

    bool rank_at_least(const Mat5& m, int rho) const
    {
        if(rho <= 0)
            return true;
        if(rho > 5)
            return false;

        const int k = 5 - rho + 1;
        const uint32_t ker = kernel_mask(m);

        for(uint32_t d : by_dim[k])
            if((d & ker) == d)
                return false;

        return true;
    }

    int rank_by_subspaces(const Mat5& m) const
    {
        const uint32_t ker = kernel_mask(m);

        for(int d = 5; d >= 0; --d)
            for(uint32_t s : by_dim[d])
                if((s & ker) == s)
                    return 5 - d;

        return -1;
    }
};

// -----------------------------------------------------------------------------
// Hardware-aware model
// -----------------------------------------------------------------------------

struct Hardware
{
    Mat5 Br;
    Mat5 Bc;
};

struct Pattern
{
    std::string name;
    Mat5 R;
    Mat5 C;
    uint8_t r0 = 0;
    uint8_t c0 = 0;
};

struct PreparedPattern
{
    Pattern p;
    Mat5 A;
    uint32_t A_code = 0;
    int rho_star = 0;
};

static int hardware_rank_ceiling(const Hardware& hw, const Pattern& p)
{
    const Mat5 BrR = compose(hw.Br, p.R);
    const Mat5 BcC = compose(hw.Bc, p.C);

    const int output_capacity = rank_hcat(BrR, hw.Bc);
    const int visible_lane_info = rank_vcat(p.R, BcC);

    return std::min(output_capacity, visible_lane_info);
}

static PreparedPattern prepare(const Hardware& hw, const Pattern& p)
{
    PreparedPattern out;
    out.p = p;
    out.A = add(compose(hw.Br, p.R), compose(hw.Bc, p.C));
    out.A_code = encode(out.A);
    out.rho_star = hardware_rank_ceiling(hw, p);
    return out;
}

static Mat5 effective_from_Q(const PreparedPattern& pp, const Mat5& q)
{
    return add(pp.A, compose(q, pp.p.R));
}

static uint32_t effective_code_from_Q_code(const PreparedPattern& pp, uint32_t q_code)
{
    // Fast path for R = I.
    static const uint32_t I_CODE = encode(identity5());
    if(encode(pp.p.R) == I_CODE)
        return pp.A_code ^ q_code;

    return pp.A_code ^ compose_code(q_code, pp.p.R);
}

static std::vector<uint8_t> image_elements(const Mat5& m)
{
    std::set<uint8_t> s;
    for(uint8_t x = 0; x < 32; ++x)
        s.insert(apply(m, x));
    return std::vector<uint8_t>(s.begin(), s.end());
}

static bool is_full_image_0_to_31(const std::vector<uint8_t>& elems)
{
    if(elems.size() != 32)
        return false;
    for(int i = 0; i < 32; ++i)
        if(elems[i] != i)
            return false;
    return true;
}

static Mat5 lift_Q_to_P(const Hardware& hw, const Mat5& q)
{
    Mat5 p;
    for(int j = 0; j < 5; ++j)
    {
        bool found = false;
        for(uint8_t x = 0; x < 32; ++x)
        {
            if(apply(hw.Bc, x) == q.col[j])
            {
                p.col[j] = x;
                found = true;
                break;
            }
        }
        if(!found)
        {
            std::cerr << "Internal error: Q column is not in Im(Bc)\n";
            std::exit(2);
        }
    }
    return p;
}

struct SolveResult
{
    bool sat = false;
    Mat5 q{};
    Mat5 p{};
    uint64_t checked = 0;
    uint64_t solutions = 0;
    double seconds = 0.0;
};

static bool candidate_ok(
    uint32_t q_code,
    const std::vector<PreparedPattern>& prepared,
    const std::vector<int>& indices)
{
    for(int idx : indices)
    {
        const auto& pp = prepared[idx];
        const uint32_t m_code = effective_code_from_Q_code(pp, q_code);
        if(rank5_code(m_code) != pp.rho_star)
            return false;
    }
    return true;
}

static bool grassmann_verify_candidate(
    const Mat5& q,
    const std::vector<PreparedPattern>& prepared,
    const std::vector<int>& indices,
    const Grassmann5& grass)
{
    for(int idx : indices)
    {
        const Mat5 m = effective_from_Q(prepared[idx], q);
        if(!grass.rank_at_least(m, prepared[idx].rho_star))
            return false;
        if(rank5(m) != prepared[idx].rho_star)
            return false;
    }
    return true;
}

static SolveResult solve_exact(
    const Hardware& hw,
    const std::vector<PreparedPattern>& prepared,
    const std::vector<int>& indices,
    const Grassmann5& grass,
    bool count_all)
{
    SolveResult result;
    const auto elems = image_elements(hw.Bc);

    const auto t0 = std::chrono::steady_clock::now();

    auto accept = [&](uint32_t q_code) {
        ++result.checked;
        if(!candidate_ok(q_code, prepared, indices))
            return false;

        Mat5 q = decode(q_code);
        if(!grassmann_verify_candidate(q, prepared, indices, grass))
        {
            std::cerr << "Internal error: Gaussian/Grassmann disagreement on solver witness.\n";
            std::exit(3);
        }

        if(!result.sat)
        {
            result.sat = true;
            result.q = q;
            result.p = lift_Q_to_P(hw, q);
        }

        ++result.solutions;
        return !count_all;
    };

    if(is_full_image_0_to_31(elems))
    {
        const uint32_t total = (1u << 25);
        for(uint32_t code = 0; code < total; ++code)
            if(accept(code))
                break;
    }
    else
    {
        uint64_t total = 1;
        for(int j = 0; j < 5; ++j)
            total *= static_cast<uint64_t>(elems.size());

        for(uint64_t idx = 0; idx < total; ++idx)
        {
            uint64_t x = idx;
            Mat5 q;
            for(int j = 0; j < 5; ++j)
            {
                const size_t digit = static_cast<size_t>(x % elems.size());
                x /= elems.size();
                q.col[j] = elems[digit];
            }
            if(accept(encode(q)))
                break;
        }
    }

    const auto t1 = std::chrono::steady_clock::now();
    result.seconds = std::chrono::duration<double>(t1 - t0).count();
    return result;
}

static std::vector<int> all_indices(int n)
{
    std::vector<int> out(n);
    std::iota(out.begin(), out.end(), 0);
    return out;
}

struct MusResult
{
    bool full_set_was_sat = false;
    std::vector<int> core;
    std::vector<std::pair<int, SolveResult>> deletion_witnesses;
    double seconds = 0.0;
};

static MusResult extract_mus(
    const Hardware& hw,
    const std::vector<PreparedPattern>& prepared,
    const Grassmann5& grass)
{
    MusResult mr;
    auto core = all_indices(static_cast<int>(prepared.size()));
    const auto start = std::chrono::steady_clock::now();

    const auto full = solve_exact(hw, prepared, core, grass, false);
    if(full.sat)
    {
        mr.full_set_was_sat = true;
        mr.core = core;
        mr.seconds =
            std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        return mr;
    }

    for(size_t pos = 0; pos < core.size();)
    {
        std::vector<int> trial = core;
        trial.erase(trial.begin() + static_cast<std::ptrdiff_t>(pos));

        const auto r = solve_exact(hw, prepared, trial, grass, false);

        if(!r.sat)
        {
            core = std::move(trial);
            // Do not increment pos: a new element moved into this position.
        }
        else
        {
            ++pos;
        }
    }

    mr.core = core;

    // Minimality witnesses: removing any remaining core member must be SAT.
    for(size_t pos = 0; pos < core.size(); ++pos)
    {
        std::vector<int> trial = core;
        const int removed = trial[pos];
        trial.erase(trial.begin() + static_cast<std::ptrdiff_t>(pos));
        const auto witness = solve_exact(hw, prepared, trial, grass, false);

        if(!witness.sat)
        {
            std::cerr << "Internal error: extracted core is not inclusion-minimal.\n";
            std::exit(4);
        }

        mr.deletion_witnesses.push_back({removed, witness});
    }

    mr.seconds =
        std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    return mr;
}

// -----------------------------------------------------------------------------
// CK case study
// -----------------------------------------------------------------------------

static Hardware canonical_ck_hardware()
{
    Hardware hw;
    hw.Br = zero5();
    hw.Bc = identity5();
    return hw;
}

static std::vector<Pattern> ck_patterns()
{
    const Mat5 I = identity5();
    const Mat5 L = shift_left1_mod32();

    std::vector<Pattern> p;

    Pattern diag;
    diag.name = "diagonal";
    diag.R = I;
    diag.C = I;
    diag.r0 = 0;
    diag.c0 = 0;
    p.push_back(diag);

    Pattern anti;
    anti.name = "anti-diagonal";
    anti.R = I;
    anti.C = I;
    anti.r0 = 0;
    anti.c0 = 31; // 31-r == 31 XOR r on five bits.
    p.push_back(anti);

    Pattern stride2;
    stride2.name = "stride-2";
    stride2.R = I;
    stride2.C = L;
    stride2.r0 = 0;
    stride2.c0 = 0;
    p.push_back(stride2);

    return p;
}

struct BankStats
{
    int distinct = 0;
    int max_multiplicity = 0;
    std::array<int, 32> count{};
};

static BankStats enumerate_ck_byte_addresses(const Pattern& pat, const Mat5& p)
{
    BankStats s;

    for(uint8_t t = 0; t < 32; ++t)
    {
        const uint8_t r = static_cast<uint8_t>(apply(pat.R, t) ^ pat.r0);
        const uint8_t c = static_cast<uint8_t>(apply(pat.C, t) ^ pat.c0);

        // CK-style lower coordinate: c' = c XOR P*r.
        const uint8_t cp = static_cast<uint8_t>(c ^ apply(p, r));

        // Relative byte address of a row-major 32x32 DWORD tile.
        const uint32_t byte_addr =
            4u * (32u * static_cast<uint32_t>(r) + static_cast<uint32_t>(cp));

        // Bank within one 32-bank RDNA3 LDS side:
        // low five bits of the DWORD address.
        const uint8_t bank = static_cast<uint8_t>((byte_addr >> 2) & 31u);

        ++s.count[bank];
    }

    for(int b = 0; b < 32; ++b)
    {
        if(s.count[b] != 0)
            ++s.distinct;
        s.max_multiplicity = std::max(s.max_multiplicity, s.count[b]);
    }

    return s;
}

static Mat5 canonical_effective(const Pattern& pat, const Mat5& p)
{
    // Br=0, Bc=I => M = C + P R.
    return add(pat.C, compose(p, pat.R));
}

static void print_case_line(const Pattern& pat, const std::string& label, const Mat5& p)
{
    const Mat5 m = canonical_effective(pat, p);
    const int r = rank5(m);
    const BankStats bs = enumerate_ck_byte_addresses(pat, p);
    const int predicted_distinct = 1 << r;
    const int predicted_mult = 1 << (5 - r);

    std::cout
        << std::left << std::setw(14) << pat.name
        << " | " << std::setw(12) << label
        << " | rank=" << r
        << " | banks=" << std::setw(2) << bs.distinct
        << " | max_mult=" << std::setw(2) << bs.max_multiplicity
        << " | predicted=(" << predicted_distinct << "," << predicted_mult << ")";

    if(bs.distinct != predicted_distinct || bs.max_multiplicity != predicted_mult)
        std::cout << "  [MISMATCH]";
    std::cout << "\n";
}

static void run_case_study()
{
    const Mat5 I = identity5();
    const Mat5 Pck = I;
    const Mat5 Pstar = ck_patch_matrix();
    const Mat5 Qstar = add(I, Pstar);

    std::cout << "=== gfx1100 / RDNA3 CK byte-address case study ===\n";
    std::cout << "Model: relative byte_addr = 4*(32*r + c'), bank32 = (byte_addr>>2)&31\n";
    std::cout << "Hardware projection: Br=0, Bc=I (effective 32-bank LDS side)\n\n";

    std::cout << "rank(P_ck=I)       = " << rank5(Pck) << "\n";
    std::cout << "rank(P_star)       = " << rank5(Pstar) << "\n";
    std::cout << "rank(I + P_star)   = " << rank5(Qstar) << "\n\n";

    for(const auto& pat : ck_patterns())
    {
        print_case_line(pat, "original", Pck);
        print_case_line(pat, "proposed", Pstar);
    }

    std::cout << "\n";
    print_matrix(Pstar, "P_star");
    print_matrix(Qstar, "I + P_star");
}

// -----------------------------------------------------------------------------
// n=2 exhaustive hardware-rank-ceiling verifier
// -----------------------------------------------------------------------------

static uint8_t mat2_apply(uint8_t code, uint8_t x)
{
    const uint8_t c0 = code & 0x3u;
    const uint8_t c1 = (code >> 2) & 0x3u;
    uint8_t y = 0;
    if(x & 1u) y ^= c0;
    if(x & 2u) y ^= c1;
    return static_cast<uint8_t>(y & 0x3u);
}

static uint8_t mat2_add(uint8_t a, uint8_t b)
{
    return static_cast<uint8_t>(a ^ b);
}

static uint8_t mat2_compose(uint8_t a, uint8_t b)
{
    uint8_t out = 0;
    const uint8_t b0 = b & 0x3u;
    const uint8_t b1 = (b >> 2) & 0x3u;
    out |= mat2_apply(a, b0);
    out |= static_cast<uint8_t>(mat2_apply(a, b1) << 2);
    return out;
}

static int rank2(uint8_t code)
{
    std::vector<uint8_t> v = {
        static_cast<uint8_t>(code & 0x3u),
        static_cast<uint8_t>((code >> 2) & 0x3u)
    };
    return rank_vectors<uint8_t>(std::move(v), 2);
}

static int rank2_hcat(uint8_t a, uint8_t b)
{
    std::vector<uint8_t> v = {
        static_cast<uint8_t>(a & 0x3u),
        static_cast<uint8_t>((a >> 2) & 0x3u),
        static_cast<uint8_t>(b & 0x3u),
        static_cast<uint8_t>((b >> 2) & 0x3u)
    };
    return rank_vectors<uint8_t>(std::move(v), 2);
}

static int rank2_vcat(uint8_t top, uint8_t bottom)
{
    std::vector<uint8_t> v;
    for(int j = 0; j < 2; ++j)
    {
        const uint8_t tc = static_cast<uint8_t>((top >> (2 * j)) & 0x3u);
        const uint8_t bc = static_cast<uint8_t>((bottom >> (2 * j)) & 0x3u);
        v.push_back(static_cast<uint8_t>(tc | (bc << 2)));
    }
    return rank_vectors<uint8_t>(std::move(v), 4);
}

static bool verify_hardware_ceiling_n2()
{
    uint64_t configs = 0;
    uint64_t evals = 0;
    uint64_t failures = 0;

    for(uint8_t Br = 0; Br < 16; ++Br)
    for(uint8_t Bc = 0; Bc < 16; ++Bc)
    for(uint8_t R = 0; R < 16; ++R)
    for(uint8_t C = 0; C < 16; ++C)
    {
        ++configs;

        const uint8_t BrR = mat2_compose(Br, R);
        const uint8_t BcC = mat2_compose(Bc, C);
        const uint8_t A = mat2_add(BrR, BcC);

        const int predicted = std::min(
            rank2_hcat(BrR, Bc),
            rank2_vcat(R, BcC));

        int actual = 0;
        for(uint8_t P = 0; P < 16; ++P)
        {
            ++evals;
            const uint8_t BcP = mat2_compose(Bc, P);
            const uint8_t BcPR = mat2_compose(BcP, R);
            const uint8_t M = mat2_add(A, BcPR);
            actual = std::max(actual, rank2(M));
        }

        if(actual != predicted)
            ++failures;
    }

    std::cout << "n=2 hardware rank-ceiling exhaustive verifier\n";
    std::cout << "  configurations: " << configs << "\n";
    std::cout << "  P evaluations:  " << evals << "\n";
    std::cout << "  failures:       " << failures << "\n";

    return failures == 0;
}

// -----------------------------------------------------------------------------
// Self-test
// -----------------------------------------------------------------------------

static bool self_test()
{
    bool ok = true;
    Grassmann5 grass;

    const std::array<size_t, 6> expected = {1, 31, 155, 155, 31, 1};

    std::cout << "Grassmann GF(2)^5 subspaces by dimension:\n";
    for(int d = 0; d <= 5; ++d)
    {
        std::cout << "  dim " << d << ": " << grass.by_dim[d].size()
                  << " (expected " << expected[d] << ")\n";
        ok = ok && (grass.by_dim[d].size() == expected[d]);
    }

    // Deterministic pseudo-random rank cross-check.
    uint32_t state = 0x12345678u;
    for(int iter = 0; iter < 10000; ++iter)
    {
        state = state * 1664525u + 1013904223u;
        const uint32_t code = state & ((1u << 25) - 1u);
        const Mat5 m = decode(code);

        const int rg = rank5(m);
        const int rs = grass.rank_by_subspaces(m);

        if(rg != rs)
        {
            std::cerr << "Gaussian/Grassmann mismatch at code " << code
                      << ": " << rg << " vs " << rs << "\n";
            ok = false;
            break;
        }
    }
    std::cout << "Gaussian vs Grassmann deterministic sweep: "
              << (ok ? "PASS" : "FAIL") << "\n";

    const Mat5 I = identity5();
    const Mat5 Pstar = ck_patch_matrix();
    const Mat5 Qstar = add(I, Pstar);

    bool mix_ok = true;
    for(uint8_t r = 0; r < 32; ++r)
        if(apply(Pstar, r) != ck_mix_scalar(r))
            mix_ok = false;

    std::cout << "rank(P_star):     " << rank5(Pstar) << " (expected 4)\n";
    std::cout << "rank(I+P_star):   " << rank5(Qstar) << " (expected 5)\n";
    std::cout << "mix matrix check: " << (mix_ok ? "PASS" : "FAIL") << "\n";

    ok = ok && (rank5(Pstar) == 4);
    ok = ok && (rank5(Qstar) == 5);
    ok = ok && mix_ok;

    const bool ceiling_ok = verify_hardware_ceiling_n2();
    ok = ok && ceiling_ok;

    std::cout << "\nSELF-TEST: " << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

// -----------------------------------------------------------------------------
// CK exact solve/count
// -----------------------------------------------------------------------------

static std::vector<PreparedPattern> prepare_all(
    const Hardware& hw,
    const std::vector<Pattern>& patterns)
{
    std::vector<PreparedPattern> out;
    for(const auto& p : patterns)
        out.push_back(prepare(hw, p));
    return out;
}

static void print_targets(const std::vector<PreparedPattern>& pp)
{
    std::cout << "Per-pattern hardware rank ceilings:\n";
    for(size_t i = 0; i < pp.size(); ++i)
        std::cout << "  [" << i << "] " << pp[i].p.name
                  << ": rho*=" << pp[i].rho_star << "\n";
}

static void solve_ck(bool count_all)
{
    const Hardware hw = canonical_ck_hardware();
    const auto patterns = ck_patterns();
    const auto prepared = prepare_all(hw, patterns);
    const auto indices = all_indices(static_cast<int>(prepared.size()));
    Grassmann5 grass;

    print_targets(prepared);

    const auto result = solve_exact(hw, prepared, indices, grass, count_all);

    std::cout << "\nQ-space candidates checked: " << result.checked << "\n";
    std::cout << std::fixed << std::setprecision(6)
              << "elapsed: " << result.seconds << " s\n";

    if(!result.sat)
    {
        std::cout << "RESULT: UNSAT\n";
        return;
    }

    std::cout << "RESULT: SAT\n";
    if(count_all)
        std::cout << "total simultaneous rank-optimal Q: "
                  << result.solutions << "\n";

    print_matrix(result.q, "first Q witness");
    print_matrix(result.p, "lifted P witness");

    std::cout << "rank(Q witness) = " << rank5(result.q) << "\n";
    std::cout << "rank(P witness) = " << rank5(result.p) << "\n";

    for(size_t i = 0; i < prepared.size(); ++i)
    {
        const Mat5 m = effective_from_Q(prepared[i], result.q);
        std::cout << "  " << prepared[i].p.name
                  << ": rank(M)=" << rank5(m)
                  << " / rho*=" << prepared[i].rho_star
                  << " / Grassmann="
                  << (grass.rank_at_least(m, prepared[i].rho_star) ? "PASS" : "FAIL")
                  << "\n";
    }
}

// -----------------------------------------------------------------------------
// MUS demo
// -----------------------------------------------------------------------------

static std::vector<Pattern> mus_demo_patterns()
{
    std::vector<Pattern> out;
    const Mat5 I = identity5();

    Pattern a0;
    a0.name = "A0=0";
    a0.R = I;
    a0.C = zero5();
    out.push_back(a0);

    // Aj = u f_j, u=e0.  Column j is e0, all other columns are zero.
    for(int j = 0; j < 5; ++j)
    {
        Pattern p;
        p.name = "A" + std::to_string(j + 1) + "=e0*f" + std::to_string(j);
        p.R = I;
        p.C = zero5();
        p.C.col[j] = 1u; // e0
        out.push_back(p);
    }

    return out;
}

static void run_mus_demo()
{
    const Hardware hw = canonical_ck_hardware();
    const auto patterns = mus_demo_patterns();
    const auto prepared = prepare_all(hw, patterns);
    Grassmann5 grass;

    std::cout << "=== six-pattern MUS demo ===\n";
    print_targets(prepared);
    std::cout << "Searching exact Q-space; this intentionally proves UNSAT by exhaustion.\n";

    const auto mr = extract_mus(hw, prepared, grass);

    if(mr.full_set_was_sat)
    {
        std::cout << "Unexpected: demo family was SAT.\n";
        return;
    }

    std::cout << "\nInclusion-minimal infeasible core size: "
              << mr.core.size() << "\n";
    for(int idx : mr.core)
        std::cout << "  [" << idx << "] " << prepared[idx].p.name << "\n";

    std::cout << "\nMinimality witnesses (remove one core member => SAT):\n";
    for(const auto& item : mr.deletion_witnesses)
    {
        const int removed = item.first;
        const auto& witness = item.second;
        std::cout << "  remove [" << removed << "] "
                  << prepared[removed].p.name
                  << " -> SAT after " << witness.checked
                  << " candidates, witness Q code=" << encode(witness.q)
                  << "\n";
    }

    std::cout << std::fixed << std::setprecision(6)
              << "\nMUS extraction elapsed: " << mr.seconds << " s\n";
}


// -----------------------------------------------------------------------------
// v2: structured robustness corpus + implementation-cost proxy
// -----------------------------------------------------------------------------

struct ShiftExpr
{
    int ops = 1 << 20;
    int shifts = 1 << 20;
    int xors = 1 << 20;
    int terms = 1 << 20;
    uint16_t mask = 0;
};

static bool better_expr(const ShiftExpr& a, const ShiftExpr& b)
{
    if(a.ops != b.ops) return a.ops < b.ops;
    if(a.shifts != b.shifts) return a.shifts < b.shifts;
    if(a.xors != b.xors) return a.xors < b.xors;
    return a.terms < b.terms;
}

static std::array<Mat5, 9> shift_primitives()
{
    std::array<Mat5, 9> p{};
    p[0] = identity5();
    for(int k = 1; k <= 4; ++k)
    {
        Mat5 l, r;
        for(int j = 0; j < 5; ++j)
        {
            l.col[j] = static_cast<uint8_t>(((1u << j) << k) & 31u);
            r.col[j] = static_cast<uint8_t>(((1u << j) >> k) & 31u);
        }
        p[k] = l;
        p[4 + k] = r;
    }
    return p;
}

static std::string shift_expr_string(uint16_t mask)
{
    static const std::array<std::string, 9> names = {
        "I", "L1", "L2", "L3", "L4", "R1", "R2", "R3", "R4"
    };

    std::ostringstream os;
    bool first = true;
    for(int i = 0; i < 9; ++i)
    {
        if((mask >> i) & 1u)
        {
            if(!first) os << " ^ ";
            os << names[i];
            first = false;
        }
    }
    if(first)
        os << "0";
    return os.str();
}

static std::unordered_map<uint32_t, ShiftExpr> build_shift_grammar()
{
    const auto prim = shift_primitives();
    std::unordered_map<uint32_t, ShiftExpr> best;
    best.reserve(1024);

    for(uint16_t mask = 0; mask < (1u << 9); ++mask)
    {
        uint32_t code = 0;
        int terms = 0;
        int shifts = 0;

        for(int i = 0; i < 9; ++i)
        {
            if((mask >> i) & 1u)
            {
                code ^= encode(prim[i]);
                ++terms;
                if(i != 0)
                    ++shifts;
            }
        }

        ShiftExpr e;
        e.terms = terms;
        e.shifts = shifts;
        e.xors = std::max(0, terms - 1);
        e.ops = e.shifts + e.xors;
        e.mask = mask;

        auto it = best.find(code);
        if(it == best.end() || better_expr(e, it->second))
            best[code] = e;
    }

    return best;
}

struct CorpusInfo
{
    std::vector<uint32_t> codes;
    size_t low_shiftxor = 0;
    size_t after_permutations = 0;
    size_t after_rank1 = 0;
    size_t after_elementary_shears = 0;
};

static CorpusInfo build_structured_access_corpus()
{
    CorpusInfo info;
    std::set<uint32_t> s;
    const auto prim = shift_primitives();

    // Complete XOR combinations of up to three terms from
    // {I, L1..L4, R1..R4}.  These are cheap shift/XOR linear couplings.
    for(int mask = 0; mask < (1 << 9); ++mask)
    {
        if(popcount32(static_cast<uint32_t>(mask)) > 3)
            continue;

        uint32_t code = 0;
        for(int i = 0; i < 9; ++i)
            if((mask >> i) & 1)
                code ^= encode(prim[i]);
        s.insert(code);
    }
    info.low_shiftxor = s.size();

    // Every bit permutation matrix: 5! = 120.
    std::array<int, 5> perm = {0,1,2,3,4};
    do
    {
        Mat5 m;
        for(int j = 0; j < 5; ++j)
            m.col[j] = static_cast<uint8_t>(1u << perm[j]);
        s.insert(encode(m));
    }
    while(std::next_permutation(perm.begin(), perm.end()));
    info.after_permutations = s.size();

    // Every rank-one coordinate map E_{out,in}.
    for(int out = 0; out < 5; ++out)
    for(int in = 0; in < 5; ++in)
    {
        Mat5 m;
        m.col[in] = static_cast<uint8_t>(1u << out);
        s.insert(encode(m));
    }
    info.after_rank1 = s.size();

    // Elementary shears I + E_{out,in}, out != in.
    const uint32_t I = encode(identity5());
    for(int out = 0; out < 5; ++out)
    for(int in = 0; in < 5; ++in)
    {
        if(out == in)
            continue;
        Mat5 e;
        e.col[in] = static_cast<uint8_t>(1u << out);
        s.insert(I ^ encode(e));
    }
    info.after_elementary_shears = s.size();

    info.codes.assign(s.begin(), s.end());
    return info;
}

struct RobustMetrics
{
    int min_rank = 5;
    int full_rank = 0;
    int rank_sum = 0;
    std::array<int, 6> hist{};
};

static RobustMetrics robustness_metrics(
    uint32_t p_code,
    const std::vector<uint32_t>& corpus,
    const std::vector<uint8_t>& rank_table)
{
    RobustMetrics m;
    for(uint32_t a : corpus)
    {
        const int r = rank_table[p_code ^ a];
        m.min_rank = std::min(m.min_rank, r);
        if(r == 5) ++m.full_rank;
        m.rank_sum += r;
        ++m.hist[r];
    }
    return m;
}

static bool robustness_better_lex(const RobustMetrics& a, const RobustMetrics& b)
{
    if(a.min_rank != b.min_rank) return a.min_rank > b.min_rank;
    if(a.full_rank != b.full_rank) return a.full_rank > b.full_rank;
    return a.rank_sum > b.rank_sum;
}

static bool robustness_dominates(const RobustMetrics& a, const RobustMetrics& b)
{
    const bool weak =
        a.min_rank >= b.min_rank &&
        a.full_rank >= b.full_rank &&
        a.rank_sum >= b.rank_sum;
    const bool strict =
        a.min_rank > b.min_rank ||
        a.full_rank > b.full_rank ||
        a.rank_sum > b.rank_sum;
    return weak && strict;
}

struct GrammarCandidate
{
    uint32_t code = 0;
    ShiftExpr expr{};
    RobustMetrics metrics{};
};

static bool grammar_dominates(const GrammarCandidate& a, const GrammarCandidate& b)
{
    const bool weak =
        a.metrics.min_rank >= b.metrics.min_rank &&
        a.metrics.full_rank >= b.metrics.full_rank &&
        a.metrics.rank_sum >= b.metrics.rank_sum &&
        a.expr.ops <= b.expr.ops;

    const bool strict =
        a.metrics.min_rank > b.metrics.min_rank ||
        a.metrics.full_rank > b.metrics.full_rank ||
        a.metrics.rank_sum > b.metrics.rank_sum ||
        a.expr.ops < b.expr.ops;

    return weak && strict;
}

static void print_metrics(const RobustMetrics& m, size_t corpus_size)
{
    std::cout
        << "min_rank=" << m.min_rank
        << ", full_rank=" << m.full_rank << "/" << corpus_size
        << ", rank_sum=" << m.rank_sum
        << ", hist={";
    for(int r = 0; r <= 5; ++r)
    {
        if(r) std::cout << ",";
        std::cout << r << ":" << m.hist[r];
    }
    std::cout << "}";
}

static std::vector<uint8_t> build_rank_table25()
{
    const uint32_t total = (1u << 25);
    std::vector<uint8_t> table(total);

    for(uint32_t code = 0; code < total; ++code)
        table[code] = static_cast<uint8_t>(rank5_code(code));

    return table;
}

static void run_robustness_ck()
{
    const auto total_start = std::chrono::steady_clock::now();

    const CorpusInfo corpus = build_structured_access_corpus();
    const auto grammar = build_shift_grammar();

    std::cout << "=== CK structured robustness spectrum v2 ===\n";
    std::cout << "Corpus construction (unique after each stage):\n";
    std::cout << "  shift/XOR <=3 terms: " << corpus.low_shiftxor << "\n";
    std::cout << "  + bit permutations:  " << corpus.after_permutations << "\n";
    std::cout << "  + rank-one maps:      " << corpus.after_rank1 << "\n";
    std::cout << "  + elementary shears:  " << corpus.after_elementary_shears << "\n";
    std::cout << "  final corpus size:     " << corpus.codes.size() << "\n";
    std::cout << "Shift/XOR implementation grammar: " << grammar.size()
              << " unique linear maps from all 2^9 primitive subsets\n\n";

    const auto rt0 = std::chrono::steady_clock::now();
    auto rank_table = build_rank_table25();
    const auto rt1 = std::chrono::steady_clock::now();
    std::cout << std::fixed << std::setprecision(6)
              << "25-bit rank table build: "
              << std::chrono::duration<double>(rt1 - rt0).count() << " s\n";

    const uint32_t I = encode(identity5());
    const uint32_t L1 = encode(shift_left1_mod32());
    const uint32_t Pstar = encode(ck_patch_matrix());

    // R=I, Br=0, Bc=I for this case study.  Anti-diagonal has the same
    // linear C=I as diagonal, so CK feasibility reduces to I and L1.
    const RobustMetrics pstar_metrics =
        robustness_metrics(Pstar, corpus.codes, rank_table);

    std::cout << "\nP_star = I ^ L3 ^ R2\n  ";
    print_metrics(pstar_metrics, corpus.codes.size());
    auto pstar_expr_it = grammar.find(Pstar);
    if(pstar_expr_it != grammar.end())
    {
        std::cout << "\n  shift/XOR proxy: ops=" << pstar_expr_it->second.ops
                  << " (shifts=" << pstar_expr_it->second.shifts
                  << ", xors=" << pstar_expr_it->second.xors
                  << "), expr=" << shift_expr_string(pstar_expr_it->second.mask);
    }
    std::cout << "\n\n";

    uint64_t feasible = 0;
    std::array<uint64_t, 6> min_rank_hist{};
    std::vector<std::vector<uint64_t>> full_by_min(6, std::vector<uint64_t>(corpus.codes.size() + 1, 0));

    bool have_best = false;
    uint32_t best_code = 0;
    RobustMetrics best_metrics{};

    bool have_robust_dominator = false;
    uint32_t robust_dominator_code = 0;
    RobustMetrics robust_dominator_metrics{};

    const auto scan0 = std::chrono::steady_clock::now();

    for(uint32_t p = 0; p < (1u << 25); ++p)
    {
        if(rank_table[p ^ I] != 5)
            continue;
        if(rank_table[p ^ L1] != 5)
            continue;

        ++feasible;
        const RobustMetrics m = robustness_metrics(p, corpus.codes, rank_table);
        ++min_rank_hist[m.min_rank];
        ++full_by_min[m.min_rank][m.full_rank];

        if(!have_best || robustness_better_lex(m, best_metrics))
        {
            have_best = true;
            best_code = p;
            best_metrics = m;
        }

        if(!have_robust_dominator && robustness_dominates(m, pstar_metrics))
        {
            have_robust_dominator = true;
            robust_dominator_code = p;
            robust_dominator_metrics = m;
        }
    }

    const auto scan1 = std::chrono::steady_clock::now();

    std::cout << "Complete CK-feasible Q/P scan:\n";
    std::cout << "  candidates in full space: " << (1u << 25) << "\n";
    std::cout << "  CK-feasible candidates:   " << feasible << "\n";
    std::cout << "  min-rank histogram over structured corpus:\n";
    for(int r = 0; r <= 5; ++r)
        if(min_rank_hist[r])
            std::cout << "    min_rank=" << r << ": " << min_rank_hist[r] << "\n";

    std::cout << "  scan elapsed: "
              << std::chrono::duration<double>(scan1 - scan0).count() << " s\n\n";

    std::cout << "Lexicographically strongest robustness witness\n"
              << "  objective: maximize (min_rank, full_rank_count, rank_sum)\n"
              << "  code=" << best_code << "\n  ";
    print_metrics(best_metrics, corpus.codes.size());
    std::cout << "\n";
    print_matrix(decode(best_code), "  matrix");

    if(have_robust_dominator)
    {
        std::cout << "\nP_star is robustness-dominated on THIS structured corpus.\n";
        std::cout << "One dominating witness: code=" << robust_dominator_code << "\n  ";
        print_metrics(robust_dominator_metrics, corpus.codes.size());
        std::cout << "\n";
    }
    else
    {
        std::cout << "\nNo CK-feasible candidate robustness-dominates P_star on this corpus.\n";
    }

    // Cost-aware analysis is deliberately restricted to a transparent,
    // directly expressible shift/XOR grammar.
    std::vector<GrammarCandidate> gfeasible;
    gfeasible.reserve(grammar.size());

    for(const auto& kv : grammar)
    {
        const uint32_t p = kv.first;
        if(rank_table[p ^ I] != 5 || rank_table[p ^ L1] != 5)
            continue;

        GrammarCandidate gc;
        gc.code = p;
        gc.expr = kv.second;
        gc.metrics = robustness_metrics(p, corpus.codes, rank_table);
        gfeasible.push_back(gc);
    }

    std::vector<GrammarCandidate> frontier;
    for(size_t i = 0; i < gfeasible.size(); ++i)
    {
        bool dominated = false;
        for(size_t j = 0; j < gfeasible.size(); ++j)
        {
            if(i == j) continue;
            if(grammar_dominates(gfeasible[j], gfeasible[i]))
            {
                dominated = true;
                break;
            }
        }
        if(!dominated)
            frontier.push_back(gfeasible[i]);
    }

    std::sort(frontier.begin(), frontier.end(),
              [](const GrammarCandidate& a, const GrammarCandidate& b) {
                  if(a.expr.ops != b.expr.ops) return a.expr.ops < b.expr.ops;
                  if(a.metrics.min_rank != b.metrics.min_rank)
                      return a.metrics.min_rank > b.metrics.min_rank;
                  if(a.metrics.full_rank != b.metrics.full_rank)
                      return a.metrics.full_rank > b.metrics.full_rank;
                  if(a.metrics.rank_sum != b.metrics.rank_sum)
                      return a.metrics.rank_sum > b.metrics.rank_sum;
                  return a.code < b.code;
              });

    std::cout << "\nShift/XOR grammar analysis:\n";
    std::cout << "  CK-feasible grammar maps: " << gfeasible.size()
              << "/" << grammar.size() << "\n";
    std::cout << "  Pareto frontier size:     " << frontier.size() << "\n";
    std::cout << "  objectives: maximize robustness tuple; minimize source-level op proxy\n\n";

    for(size_t i = 0; i < frontier.size(); ++i)
    {
        const auto& g = frontier[i];
        std::cout << "  [" << i << "] code=" << g.code
                  << ", ops=" << g.expr.ops
                  << " (shifts=" << g.expr.shifts
                  << ", xors=" << g.expr.xors << ")"
                  << ", expr=" << shift_expr_string(g.expr.mask)
                  << "\n      ";
        print_metrics(g.metrics, corpus.codes.size());
        std::cout << "\n";
    }

    bool pstar_on_frontier = false;
    GrammarCandidate pstar_gc;
    if(pstar_expr_it != grammar.end())
    {
        pstar_gc.code = Pstar;
        pstar_gc.expr = pstar_expr_it->second;
        pstar_gc.metrics = pstar_metrics;

        for(const auto& g : frontier)
            if(g.code == Pstar)
                pstar_on_frontier = true;
    }

    std::cout << "\nP_star on shift/XOR robustness-cost Pareto frontier: "
              << (pstar_on_frontier ? "YES" : "NO") << "\n";

    if(!pstar_on_frontier && pstar_expr_it != grammar.end())
    {
        for(const auto& g : gfeasible)
        {
            if(grammar_dominates(g, pstar_gc))
            {
                std::cout << "Example grammar candidate dominating P_star:\n"
                          << "  code=" << g.code
                          << ", ops=" << g.expr.ops
                          << ", expr=" << shift_expr_string(g.expr.mask)
                          << "\n  ";
                print_metrics(g.metrics, corpus.codes.size());
                std::cout << "\n";
                break;
            }
        }
    }

    const auto total_end = std::chrono::steady_clock::now();
    std::cout << "\nTOTAL robustness analysis elapsed: "
              << std::chrono::duration<double>(total_end - total_start).count()
              << " s\n";

    std::cout << "\nInterpretation boundary:\n"
              << "  This corpus is a declared non-uniform stress family, not a universal access prior.\n"
              << "  Over the complete uniform matrix universe A in Mat(5,2), A -> A+P is a bijection,\n"
              << "  so every fixed P has exactly the same global rank histogram.\n"
              << "  Therefore candidate selection beyond feasibility necessarily requires either a\n"
              << "  workload/access prior or a separate implementation-cost objective.\n";
}


// -----------------------------------------------------------------------------
// CLI
// -----------------------------------------------------------------------------

static void help(const char* argv0)
{
    std::cout
        << "Qingming Affine Shear exact_synth v2\n\n"
        << "Usage:\n"
        << "  " << argv0 << " --self-test\n"
        << "  " << argv0 << " --case-study\n"
        << "  " << argv0 << " --solve-ck\n"
        << "  " << argv0 << " --count-ck\n"
        << "  " << argv0 << " --mus-demo\n"
        << "  " << argv0 << " --robustness-ck\n"
        << "  " << argv0 << " --help\n\n"
        << "Notes:\n"
        << "  --solve-ck stops at the first simultaneous rank-optimal Q.\n"
        << "  --count-ck exhausts the complete Q-space and counts all solutions.\n"
        << "  --mus-demo exhausts Q-space as needed to prove infeasibility and minimality.\n"
        << "  --robustness-ck scans all CK-feasible maps against a declared structured access corpus.\n";
}

} // namespace qas

int main(int argc, char** argv)
{
    using namespace qas;

    if(argc != 2)
    {
        help(argv[0]);
        return argc == 1 ? 0 : 1;
    }

    const std::string arg = argv[1];

    if(arg == "--self-test")
        return self_test() ? 0 : 2;
    if(arg == "--case-study")
    {
        run_case_study();
        return 0;
    }
    if(arg == "--solve-ck")
    {
        solve_ck(false);
        return 0;
    }
    if(arg == "--count-ck")
    {
        solve_ck(true);
        return 0;
    }
    if(arg == "--mus-demo")
    {
        run_mus_demo();
        return 0;
    }
    if(arg == "--robustness-ck")
    {
        run_robustness_ck();
        return 0;
    }
    if(arg == "--help" || arg == "-h")
    {
        help(argv[0]);
        return 0;
    }

    std::cerr << "Unknown option: " << arg << "\n\n";
    help(argv[0]);
    return 1;
}
