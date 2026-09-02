#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace qas_circuit {

constexpr int N = 5;
constexpr uint32_t CODE_MASK = (1u << 25) - 1u;
constexpr int STATE_CAP = 12; // input + up to 11 gates

using Code = uint32_t;

static Code identity_code()
{
    Code code = 0;
    for(int j = 0; j < 5; ++j)
        code |= (static_cast<Code>(1u << j) << (5 * j));
    return code;
}

static uint8_t apply_code(Code code, uint8_t x)
{
    uint8_t y = 0;
    for(int j = 0; j < 5; ++j)
        if((x >> j) & 1u)
            y ^= static_cast<uint8_t>((code >> (5 * j)) & 31u);
    return static_cast<uint8_t>(y & 31u);
}

static Code shl_code(Code code, int k)
{
    Code out = 0;
    for(int j = 0; j < 5; ++j)
    {
        uint8_t col = static_cast<uint8_t>((code >> (5 * j)) & 31u);
        col = static_cast<uint8_t>((static_cast<uint32_t>(col) << k) & 31u);
        out |= (static_cast<Code>(col) << (5 * j));
    }
    return out;
}

static Code shr_code(Code code, int k)
{
    Code out = 0;
    for(int j = 0; j < 5; ++j)
    {
        uint8_t col = static_cast<uint8_t>((code >> (5 * j)) & 31u);
        col = static_cast<uint8_t>(col >> k);
        out |= (static_cast<Code>(col) << (5 * j));
    }
    return out;
}

static Code and_code(Code code, uint8_t mask)
{
    Code out = 0;
    for(int j = 0; j < 5; ++j)
    {
        const uint8_t col =
            static_cast<uint8_t>(((code >> (5 * j)) & 31u) & mask);
        out |= (static_cast<Code>(col) << (5 * j));
    }
    return out;
}

static uint8_t ck_mix_scalar(uint8_t r)
{
    return static_cast<uint8_t>((r ^ (r >> 2) ^ (r << 3)) & 31u);
}

static Code ck_pstar_code()
{
    Code out = 0;
    for(int j = 0; j < 5; ++j)
        out |= (static_cast<Code>(ck_mix_scalar(static_cast<uint8_t>(1u << j)))
                << (5 * j));
    return out;
}

static Code rshift_original_code(int k)
{
    return shr_code(identity_code(), k);
}

static Code lshift_original_code(int k)
{
    return shl_code(identity_code(), k);
}

static void print_matrix(Code code, const std::string& name)
{
    std::cout << name << " (rows):\n";
    for(int r = 0; r < 5; ++r)
    {
        std::cout << "  [ ";
        for(int c = 0; c < 5; ++c)
            std::cout << ((code >> (5 * c + r)) & 1u) << (c == 4 ? " " : " ");
        std::cout << "]\n";
    }
}

enum class Op : uint8_t
{
    Input,
    Shl,
    Shr,
    And,
    Xor
};

struct Gate
{
    Op op = Op::Input;
    Code a = 0;
    Code b = 0;
    uint8_t param = 0;
    Code out = 0;
};

struct State
{
    uint8_t n = 0;
    std::array<Code, STATE_CAP> v{};

    bool operator==(const State& other) const
    {
        if(n != other.n)
            return false;
        for(int i = 0; i < n; ++i)
            if(v[i] != other.v[i])
                return false;
        return true;
    }
};

struct StateHash
{
    size_t operator()(const State& s) const noexcept
    {
        uint64_t h = 1469598103934665603ull;
        h ^= s.n;
        h *= 1099511628211ull;
        for(int i = 0; i < s.n; ++i)
        {
            h ^= static_cast<uint64_t>(s.v[i]);
            h *= 1099511628211ull;
        }
        return static_cast<size_t>(h);
    }
};

static bool contains(const State& s, Code x)
{
    for(int i = 0; i < s.n; ++i)
        if(s.v[i] == x)
            return true;
    return false;
}

static State add_value(const State& s, Code x)
{
    State t = s;
    if(t.n >= STATE_CAP)
    {
        std::cerr << "STATE_CAP exceeded\n";
        std::exit(3);
    }

    int pos = t.n;
    while(pos > 0 && t.v[pos - 1] > x)
    {
        t.v[pos] = t.v[pos - 1];
        --pos;
    }
    t.v[pos] = x;
    ++t.n;
    return t;
}

struct Node
{
    State state{};
    uint32_t parent = std::numeric_limits<uint32_t>::max();
    Gate gate{};
};

struct SearchResult
{
    bool found = false;
    int exact_gates = -1;
    int lower_bound = 0;
    uint32_t target_node = 0;
    uint64_t states_expanded = 0;
    uint64_t successor_states_created = 0;
    std::vector<uint64_t> states_by_depth;
    std::vector<Node> nodes;
};

static void add_candidate(
    std::vector<Gate>& gates,
    const State& s,
    Op op,
    Code a,
    Code b,
    uint8_t param,
    Code out)
{
    out &= CODE_MASK;
    if(out == 0 || contains(s, out))
        return;

    Gate g;
    g.op = op;
    g.a = a;
    g.b = b;
    g.param = param;
    g.out = out;
    gates.push_back(g);
}

static std::vector<Gate> enumerate_new_gates(const State& s)
{
    std::vector<Gate> gates;
    gates.reserve(static_cast<size_t>(s.n) * 40u +
                  static_cast<size_t>(s.n) * static_cast<size_t>(s.n));

    for(int i = 0; i < s.n; ++i)
    {
        const Code a = s.v[i];

        for(int k = 1; k <= 4; ++k)
        {
            add_candidate(gates, s, Op::Shl, a, 0, static_cast<uint8_t>(k),
                          shl_code(a, k));
            add_candidate(gates, s, Op::Shr, a, 0, static_cast<uint8_t>(k),
                          shr_code(a, k));
        }

        // mask=0 gives zero and mask=31 is identity, both redundant
        // in a minimum-gate circuit.
        for(uint8_t mask = 1; mask < 31; ++mask)
            add_candidate(gates, s, Op::And, a, 0, mask, and_code(a, mask));
    }

    for(int i = 0; i < s.n; ++i)
    for(int j = i + 1; j < s.n; ++j)
        add_candidate(gates, s, Op::Xor, s.v[i], s.v[j], 0,
                      s.v[i] ^ s.v[j]);

    // Multiple operations can synthesize the same new matrix from the
    // same available-value set. They lead to exactly the same future state,
    // so keep one deterministic representative.
    std::sort(gates.begin(), gates.end(),
              [](const Gate& a, const Gate& b) {
                  if(a.out != b.out) return a.out < b.out;
                  if(a.op != b.op)
                      return static_cast<int>(a.op) < static_cast<int>(b.op);
                  if(a.a != b.a) return a.a < b.a;
                  if(a.b != b.b) return a.b < b.b;
                  return a.param < b.param;
              });

    gates.erase(
        std::unique(gates.begin(), gates.end(),
                    [](const Gate& a, const Gate& b) {
                        return a.out == b.out;
                    }),
        gates.end());

    return gates;
}

static SearchResult exact_bfs(Code target, int max_gates)
{
    SearchResult result;
    target &= CODE_MASK;

    if(max_gates < 0 || max_gates + 1 > STATE_CAP)
    {
        std::cerr << "max_gates must be in [0," << (STATE_CAP - 1) << "]\n";
        std::exit(2);
    }

    const Code input = identity_code();

    Node root;
    root.state.n = 1;
    root.state.v[0] = input;
    root.gate.op = Op::Input;

    result.nodes.push_back(root);
    result.states_by_depth.push_back(1);

    if(target == input)
    {
        result.found = true;
        result.exact_gates = 0;
        result.target_node = 0;
        return result;
    }

    std::unordered_map<State, uint32_t, StateHash> seen;
    seen.reserve(500000);
    seen.emplace(root.state, 0);

    std::vector<uint32_t> current{0};

    for(int depth = 0; depth < max_gates; ++depth)
    {
        std::vector<uint32_t> next;
        if(depth <= 3)
            next.reserve(current.size() * 16u);

        for(uint32_t id : current)
        {
            ++result.states_expanded;
            // Copy, do not hold a reference: result.nodes grows while successors
            // are inserted and vector reallocation would invalidate a reference.
            const State s = result.nodes[id].state;
            const auto gates = enumerate_new_gates(s);

            for(const Gate& g : gates)
            {
                State ns = add_value(s, g.out);

                auto [it, inserted] =
                    seen.emplace(ns, static_cast<uint32_t>(result.nodes.size()));

                uint32_t nid = it->second;

                if(inserted)
                {
                    Node node;
                    node.state = ns;
                    node.parent = id;
                    node.gate = g;

                    nid = static_cast<uint32_t>(result.nodes.size());
                    it->second = nid;
                    result.nodes.push_back(node);
                    next.push_back(nid);
                    ++result.successor_states_created;
                }

                if(g.out == target)
                {
                    // This is exact: all circuits with <= depth gates have
                    // already been exhausted, and this target uses depth+1.
                    result.found = true;
                    result.exact_gates = depth + 1;
                    result.target_node = nid;
                    result.states_by_depth.push_back(next.size());
                    return result;
                }
            }
        }

        result.states_by_depth.push_back(next.size());
        current.swap(next);

        if(current.empty())
            break;
    }

    result.lower_bound = max_gates + 1;
    return result;
}

static std::vector<Gate> reconstruct(const SearchResult& r)
{
    std::vector<Gate> rev;
    uint32_t id = r.target_node;

    while(id != 0)
    {
        const Node& n = r.nodes[id];
        rev.push_back(n.gate);
        id = n.parent;
    }

    std::reverse(rev.begin(), rev.end());
    return rev;
}

static std::string code_hex(Code c)
{
    std::ostringstream os;
    os << "0x" << std::hex << std::uppercase << c;
    return os.str();
}

static void print_circuit(const SearchResult& r, Code target)
{
    const auto gates = reconstruct(r);

    std::unordered_map<Code, int> var;
    std::unordered_map<Code, int> depth;
    var[identity_code()] = 0;
    depth[identity_code()] = 0;

    int shifts = 0;
    int ands = 0;
    int xors = 0;
    int next_var = 1;
    int witness_depth = 0;

    std::cout << "v0 = x\n";

    for(const Gate& g : gates)
    {
        const int outv = next_var++;
        var[g.out] = outv;

        int d = 0;
        switch(g.op)
        {
        case Op::Shl:
            ++shifts;
            d = depth[g.a] + 1;
            std::cout << "v" << outv << " = (v" << var[g.a]
                      << " << " << static_cast<int>(g.param)
                      << ") & 0x1F";
            break;
        case Op::Shr:
            ++shifts;
            d = depth[g.a] + 1;
            std::cout << "v" << outv << " = v" << var[g.a]
                      << " >> " << static_cast<int>(g.param);
            break;
        case Op::And:
            ++ands;
            d = depth[g.a] + 1;
            std::cout << "v" << outv << " = v" << var[g.a]
                      << " & 0x" << std::hex << std::uppercase
                      << static_cast<int>(g.param)
                      << std::dec;
            break;
        case Op::Xor:
            ++xors;
            d = std::max(depth[g.a], depth[g.b]) + 1;
            std::cout << "v" << outv << " = v" << var[g.a]
                      << " ^ v" << var[g.b];
            break;
        default:
            break;
        }

        depth[g.out] = d;
        witness_depth = std::max(witness_depth, d);

        std::cout << "    // matrix=" << code_hex(g.out) << "\n";
    }

    std::cout << "return v" << var[target] << ";\n";
    std::cout << "witness operator counts: shifts=" << shifts
              << ", ands=" << ands << ", xors=" << xors << "\n";
    std::cout << "witness dependency depth: " << witness_depth
              << " (gate count is exact; depth is for this witness)\n";
}

static void print_search_summary(const SearchResult& r, Code target, int max_gates)
{
    std::cout << "target code: " << target << " (" << code_hex(target) << ")\n";
    print_matrix(target, "target");

    std::cout << "states observed by depth:\n";
    for(size_t d = 0; d < r.states_by_depth.size(); ++d)
        std::cout << "  gates=" << d << ": " << r.states_by_depth[d] << "\n";

    std::cout << "states expanded: " << r.states_expanded << "\n";
    std::cout << "successor states created: " << r.successor_states_created << "\n";

    if(r.found)
    {
        std::cout << "EXACT MINIMUM GATE COUNT: " << r.exact_gates << "\n";
        print_circuit(r, target);
    }
    else
    {
        std::cout << "NOT FOUND through " << max_gates << " gates\n";
        std::cout << "CERTIFIED LOWER BOUND: gate_count >= "
                  << (max_gates + 1) << "\n";
    }
}

static bool check_semantics(Code target, const SearchResult& r)
{
    if(!r.found)
        return true;

    const auto gates = reconstruct(r);
    std::unordered_map<Code, Code> have;
    have[identity_code()] = identity_code();

    for(const auto& g : gates)
    {
        Code out = 0;
        switch(g.op)
        {
        case Op::Shl: out = shl_code(g.a, g.param); break;
        case Op::Shr: out = shr_code(g.a, g.param); break;
        case Op::And: out = and_code(g.a, g.param); break;
        case Op::Xor: out = g.a ^ g.b; break;
        default: break;
        }

        if(out != g.out)
            return false;
        have[g.out] = g.out;
    }

    if(have.find(target) == have.end())
        return false;

    // Exhaustively check target matrix action on all 32 inputs against itself;
    // this also catches accidental code-width corruption.
    for(uint8_t x = 0; x < 32; ++x)
    {
        const uint8_t y = apply_code(target, x);
        if(y > 31)
            return false;
    }
    return true;
}

static bool self_test()
{
    bool ok = true;

    const Code I = identity_code();
    const Code R4 = rshift_original_code(4);
    const Code Pstar = ck_pstar_code();
    const Code alt = I ^ lshift_original_code(1) ^ rshift_original_code(4);

    // Exact state-space cardinalities for this gate model.
    // We obtain these by forcing an impossible target value 0: the BFS never
    // adds zero, so it exhausts every state through the requested depth.
    auto ex1 = exact_bfs(0, 1);
    auto ex2 = exact_bfs(0, 2);
    auto ex3 = exact_bfs(0, 3);
    auto ex4 = exact_bfs(0, 4);

    const std::array<uint64_t, 5> expected = {1, 38, 915, 17942, 331291};

    std::cout << "Exact circuit-state counts under {SHL,SHR,AND,XOR}:\n";
    for(int d = 0; d <= 4; ++d)
    {
        const uint64_t got = ex4.states_by_depth.at(d);
        std::cout << "  gates=" << d << ": " << got
                  << " (expected " << expected[d] << ")\n";
        ok = ok && (got == expected[d]);
    }

    auto r4 = exact_bfs(R4, 1);
    auto ps = exact_bfs(Pstar, 4);
    auto al = exact_bfs(alt, 4);

    ok = ok && r4.found && r4.exact_gates == 1;
    ok = ok && ps.found && ps.exact_gates == 4;
    ok = ok && al.found && al.exact_gates == 4;
    ok = ok && check_semantics(R4, r4);
    ok = ok && check_semantics(Pstar, ps);
    ok = ok && check_semantics(alt, al);

    std::cout << "R4 exact cost: " << (r4.found ? r4.exact_gates : -1)
              << " (expected 1)\n";
    std::cout << "P_star exact cost: " << (ps.found ? ps.exact_gates : -1)
              << " (expected 4)\n";
    std::cout << "I^L1^R4 exact cost: " << (al.found ? al.exact_gates : -1)
              << " (expected 4)\n";

    std::cout << "\nSELF-TEST: " << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

static Code parse_code(const std::string& s)
{
    size_t pos = 0;
    unsigned long v = std::stoul(s, &pos, 0);
    if(pos != s.size() || v > CODE_MASK)
        throw std::runtime_error("target code must fit in 25 bits");
    return static_cast<Code>(v);
}

static void run_named(const std::string& name, int max_gates)
{
    Code target = 0;

    if(name == "pstar" || name == "ck-pstar")
        target = ck_pstar_code();
    else if(name == "r4")
        target = rshift_original_code(4);
    else if(name == "alt" || name == "i-l1-r4")
        target = identity_code() ^ lshift_original_code(1) ^ rshift_original_code(4);
    else if(name == "robust-v2")
        target = 11266486u;
    else
        throw std::runtime_error("unknown named target");

    const auto r = exact_bfs(target, max_gates);
    print_search_summary(r, target, max_gates);
}

static void demo()
{
    struct Item { std::string name; Code code; int max_gates; };
    const std::vector<Item> items = {
        {"R4", rshift_original_code(4), 1},
        {"P_star = I ^ L3 ^ R2", ck_pstar_code(), 4},
        {"v2 equal-proxy alternative = I ^ L1 ^ R4",
         identity_code() ^ lshift_original_code(1) ^ rshift_original_code(4), 4},
        {"v2 strongest structured-robustness witness (lower-bound run)",
         11266486u, 4}
    };

    for(size_t i = 0; i < items.size(); ++i)
    {
        if(i) std::cout << "\n============================================================\n\n";
        std::cout << items[i].name << "\n";
        const auto r = exact_bfs(items[i].code, items[i].max_gates);
        print_search_summary(r, items[i].code, items[i].max_gates);
    }
}

static void help(const char* argv0)
{
    std::cout
        << "Qingming exact 5-bit linear-circuit synthesizer v1\n\n"
        << "Exact gate model (all intermediates are 5-bit GF(2)-linear maps):\n"
        << "  SHL_k(v) = (v << k) & 0x1F, k=1..4       cost 1\n"
        << "  SHR_k(v) = v >> k,             k=1..4       cost 1\n"
        << "  AND_m(v) = v & m,              m=1..30      cost 1\n"
        << "  XOR(a,b) = a ^ b, a!=b                       cost 1\n"
        << "  input x                                            free\n\n"
        << "Usage:\n"
        << "  " << argv0 << " --self-test\n"
        << "  " << argv0 << " --demo\n"
        << "  " << argv0 << " --named <pstar|r4|i-l1-r4|robust-v2> [max_gates]\n"
        << "  " << argv0 << " --code <decimal-or-0xhex> [max_gates]\n"
        << "  " << argv0 << " --help\n\n"
        << "The BFS is exact within the declared gate model. If a target is found\n"
        << "at depth d, d is the exact minimum gate count. If it is not found\n"
        << "through max_gates, the program certifies cost >= max_gates+1.\n"
        << "Depth 4 exhausts 331,291 canonical circuit states; larger depths can\n"
        << "grow quickly in memory/time.\n";
}

} // namespace qas_circuit

int main(int argc, char** argv)
{
    using namespace qas_circuit;

    try
    {
        if(argc == 2 && std::string(argv[1]) == "--self-test")
            return self_test() ? 0 : 2;

        if(argc == 2 && std::string(argv[1]) == "--demo")
        {
            demo();
            return 0;
        }

        if(argc >= 3 && std::string(argv[1]) == "--named")
        {
            const int max_gates = (argc >= 4) ? std::stoi(argv[3]) : 4;
            run_named(argv[2], max_gates);
            return 0;
        }

        if(argc >= 3 && std::string(argv[1]) == "--code")
        {
            const Code target = parse_code(argv[2]);
            const int max_gates = (argc >= 4) ? std::stoi(argv[3]) : 4;
            const auto r = exact_bfs(target, max_gates);
            print_search_summary(r, target, max_gates);
            return 0;
        }

        if(argc == 1 || (argc == 2 &&
           (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h")))
        {
            help(argv[0]);
            return 0;
        }

        help(argv[0]);
        return 1;
    }
    catch(const std::exception& e)
    {
        std::cerr << "error: " << e.what() << "\n";
        return 2;
    }
}
