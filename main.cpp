// ============================================================
//  bfs_tree.cpp
//  Parcours en largeur (Breadth-First Search) d'un arbre binaire
//  - exemple affiché visuellement
//  - BFS par niveau
//  - comparaison std::queue / std::deque / vector + index
//  - mesure de temps et scaling
// ============================================================

#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <queue>
#include <deque>
#include <chrono>
#include <algorithm>

// ------------------------------------------------------------
// Timer haute résolution
// ------------------------------------------------------------
class Timer {
    using Clock = std::chrono::high_resolution_clock;
    Clock::time_point m_start, m_end;
public:
    void start() { m_start = Clock::now(); }
    void stop()  { m_end   = Clock::now(); }
    double us() const {
        return std::chrono::duration<double, std::micro>(m_end - m_start).count();
    }
};

// ------------------------------------------------------------
// Formatage lisible des temps
// ------------------------------------------------------------
std::string formatTime(double us) {
    std::ostringstream oss;
    oss << std::fixed;
    if (us >= 1'000'000.0)  oss << std::setprecision(3) << (us / 1'000'000.0) << " s";
    else if (us >= 1'000.0) oss << std::setprecision(3) << (us / 1'000.0)     << " ms";
    else                    oss << std::setprecision(2) << us                 << " us";
    return oss.str();
}

// ------------------------------------------------------------
// Empêche l'optimiseur de supprimer l'appel
// ------------------------------------------------------------
template <typename T>
inline void doNotOptimize(const T& v) {
#if defined(__GNUC__) || defined(__clang__)
    asm volatile("" : : "r,m"(v) : "memory");
#else
    volatile auto sink = v;
    (void)sink;
#endif
}

// ============================================================
// Arbre binaire
// ============================================================
struct TreeNode {
    int val;
    TreeNode* left;
    TreeNode* right;
    explicit TreeNode(int v) : val(v), left(nullptr), right(nullptr) {}
};

// ------------------------------------------------------------
// Construction de l'arbre d'exemple
//
//            1
//           / \
//          2   3
//         / \ / \
//        4  5 6  7
//       / \
//      8   9
// ------------------------------------------------------------
TreeNode* buildExampleTree() {
    auto n1 = new TreeNode(1);
    auto n2 = new TreeNode(2);
    auto n3 = new TreeNode(3);
    auto n4 = new TreeNode(4);
    auto n5 = new TreeNode(5);
    auto n6 = new TreeNode(6);
    auto n7 = new TreeNode(7);
    auto n8 = new TreeNode(8);
    auto n9 = new TreeNode(9);
    n1->left = n2; n1->right = n3;
    n2->left = n4; n2->right = n5;
    n3->left = n6; n3->right = n7;
    n4->left = n8; n4->right = n9;
    return n1;
}

// Arbre binaire complet de n noeuds (construction itérative, pas de récursion)
TreeNode* buildCompleteTree(int n) {
    if (n <= 0) return nullptr;
    std::vector<TreeNode*> nodes(n);
    for (int i = 0; i < n; ++i) nodes[i] = new TreeNode(i + 1);
    for (int i = 0; i < n; ++i) {
        int l = 2 * i + 1, r = 2 * i + 2;
        if (l < n) nodes[i]->left  = nodes[l];
        if (r < n) nodes[i]->right = nodes[r];
    }
    return nodes[0];
}

// Libération itérative (évite le stack overflow sur grands arbres)
void freeTree(TreeNode* root) {
    if (!root) return;
    std::vector<TreeNode*> stack;
    stack.push_back(root);
    while (!stack.empty()) {
        TreeNode* n = stack.back();
        stack.pop_back();
        if (n->left)  stack.push_back(n->left);
        if (n->right) stack.push_back(n->right);
        delete n;
    }
}

// ============================================================
// Affichage arborescent ASCII
// ============================================================
void printTreeHelper(TreeNode* n, const std::string& prefix, bool isLast) {
    if (!n) return;
    std::cout << prefix << (isLast ? "'--- " : "|--- ") << n->val << "\n";
    std::string childPrefix = prefix + (isLast ? "    " : "|   ");
    std::vector<TreeNode*> children;
    if (n->left)  children.push_back(n->left);
    if (n->right) children.push_back(n->right);
    for (std::size_t i = 0; i < children.size(); ++i)
        printTreeHelper(children[i], childPrefix, i + 1 == children.size());
}

void printTree(TreeNode* root) {
    if (!root) return;
    std::cout << root->val << "\n";
    std::vector<TreeNode*> children;
    if (root->left)  children.push_back(root->left);
    if (root->right) children.push_back(root->right);
    for (std::size_t i = 0; i < children.size(); ++i)
        printTreeHelper(children[i], "", i + 1 == children.size());
}

// ============================================================
// BFS — implémentations avec 3 contenaires différents
// ============================================================

// --- 1) std::queue (le choix idiomatique) ---
std::vector<int> bfsQueue(TreeNode* root) {
    std::vector<int> result;
    if (!root) return result;
    std::queue<TreeNode*> q;
    q.push(root);
    while (!q.empty()) {
        TreeNode* n = q.front(); q.pop();
        result.push_back(n->val);
        if (n->left)  q.push(n->left);
        if (n->right) q.push(n->right);
    }
    return result;
}

// --- 2) std::deque directe ---
std::vector<int> bfsDeque(TreeNode* root) {
    std::vector<int> result;
    if (!root) return result;
    std::deque<TreeNode*> dq;
    dq.push_back(root);
    while (!dq.empty()) {
        TreeNode* n = dq.front(); dq.pop_front();
        result.push_back(n->val);
        if (n->left)  dq.push_back(n->left);
        if (n->right) dq.push_back(n->right);
    }
    return result;
}

// --- 3) vector + index (queue manuelle, sans pop_front) ---
std::vector<int> bfsVectorQueue(TreeNode* root) {
    std::vector<int> result;
    if (!root) return result;
    std::vector<TreeNode*> q;
    q.reserve(64);
    q.push_back(root);
    std::size_t head = 0;
    while (head < q.size()) {
        TreeNode* n = q[head++];
        result.push_back(n->val);
        if (n->left)  q.push_back(n->left);
        if (n->right) q.push_back(n->right);
    }
    return result;
}

// --- BFS avec affichage niveau par niveau ---
void bfsByLevel(TreeNode* root) {
    if (!root) return;
    std::queue<TreeNode*> q;
    q.push(root);
    int level = 0;
    while (!q.empty()) {
        const std::size_t sz = q.size();
        std::cout << "  Niveau " << level << " : ";
        for (std::size_t i = 0; i < sz; ++i) {
            TreeNode* n = q.front(); q.pop();
            std::cout << n->val << " ";
            if (n->left)  q.push(n->left);
            if (n->right) q.push(n->right);
        }
        std::cout << "\n";
        ++level;
    }
}

// ============================================================
// Benchmark
// ============================================================
template <typename Fn>
double benchmark(Fn fn, TreeNode* root, int iterations) {
    Timer t;
    t.start();
    for (int i = 0; i < iterations; ++i) {
        auto r = fn(root);
        doNotOptimize(r.size());
    }
    t.stop();
    return t.us() / iterations;    // temps moyen par parcours, en us
}

// ============================================================
// Main
// ============================================================
int main()
{
    // ------------------------------------------------------
    // PARTIE 1 : arbre d'exemple
    // ------------------------------------------------------
    std::cout << "========================================================\n";
    std::cout << "  PARCOURS EN LARGEUR (BFS) — EXEMPLE\n";
    std::cout << "========================================================\n\n";

    TreeNode* example = buildExampleTree();

    std::cout << "Arbre :\n";
    printTree(example);
    std::cout << "\n";

    std::cout << "Parcours en largeur par niveau :\n";
    bfsByLevel(example);
    std::cout << "\n";

    auto seq = bfsQueue(example);
    std::cout << "Sequence BFS complete : ";
    for (int v : seq) std::cout << v << " ";
    std::cout << "\n\n";

    // ------------------------------------------------------
    // PARTIE 2 : comparaison des contenaires
    // ------------------------------------------------------
    std::cout << "========================================================\n";
    std::cout << "  COMPARAISON DES CONTENAIRES (petit arbre, N=9)\n";
    std::cout << "========================================================\n\n";

    const int ITER_SMALL = 100'000;
    std::cout << "  Iterations par variante : " << ITER_SMALL << "\n";
    std::cout << "  " << std::left << std::setw(20) << "Conteneur"
              << std::setw(18) << "Temps moyen"
              << "Sequence identique ?\n";
    std::cout << "  " << std::string(60, '-') << "\n";

    auto ref = bfsQueue(example);
    auto check = [&](const std::vector<int>& v) {
        return (v == ref) ? "oui" : "NON";
    };

    double tQ = benchmark(bfsQueue,       example, ITER_SMALL);
    double tD = benchmark(bfsDeque,       example, ITER_SMALL);
    double tV = benchmark(bfsVectorQueue, example, ITER_SMALL);

    std::cout << "  " << std::left << std::setw(20) << "std::queue"
              << std::setw(18) << formatTime(tQ)
              << check(bfsQueue(example))       << "\n";
    std::cout << "  " << std::left << std::setw(20) << "std::deque"
              << std::setw(18) << formatTime(tD)
              << check(bfsDeque(example))       << "\n";
    std::cout << "  " << std::left << std::setw(20) << "vector + index"
              << std::setw(18) << formatTime(tV)
              << check(bfsVectorQueue(example)) << "\n\n";

    freeTree(example);

    // ------------------------------------------------------
    // PARTIE 3 : scaling
    // ------------------------------------------------------
    std::cout << "========================================================\n";
    std::cout << "  SCALING : BFS sur arbres complets de taille croissante\n";
    std::cout << "========================================================\n\n";

    std::cout << "  " << std::left << std::setw(12) << "Noeuds"
              << std::setw(12) << "Hauteur"
              << std::setw(18) << "Temps BFS"
              << "Debit\n";
    std::cout << "  " << std::string(64, '-') << "\n";

    for (int n : {1'000, 10'000, 100'000, 1'000'000, 5'000'000}) {
        TreeNode* tree = buildCompleteTree(n);

        int height = 0;
        for (int x = n; x > 0; x >>= 1) ++height;

        // Plus l'arbre est petit, plus on itere pour un temps mesurable
        int iterations = std::max(1, 200'000 / n);

        double t = benchmark(bfsQueue, tree, iterations);
        double throughput = n / t;   // noeuds par microseconde

        std::cout << "  " << std::left << std::setw(12) << n
                  << std::setw(12) << height
                  << std::setw(18) << formatTime(t)
                  << std::fixed << std::setprecision(2)
                  << throughput << " noeuds/us\n";

        freeTree(tree);
    }

    // ------------------------------------------------------
    // Conclusion
    // ------------------------------------------------------
    std::cout << "\n========================================================\n";
    std::cout << "  INTERPRETATION\n";
    std::cout << "========================================================\n";
    std::cout << "  - BFS parcourt l'arbre niveau par niveau, O(n) total.\n";
    std::cout << "  - std::queue utilise std::deque en interne :\n";
    std::cout << "      push/pop aux deux extremites en O(1) amorti.\n";
    std::cout << "  - vector + index evite le pop_front (decalage) :\n";
    std::cout << "      il avance juste un index -> tres rapide, mais\n";
    std::cout << "      garde tous les elements en memoire jusqu'a la fin.\n";
    std::cout << "  - Sur grands arbres, les trois variantes convergent :\n";
    std::cout << "      le cout dominant est la lecture memoire des noeuds\n";
    std::cout << "      (cache misses), pas la structure de file.\n";
    std::cout << "========================================================\n";

    return 0;
}
