#include "tst_framework.h"

#include <LangCore/Module/Dependency/DependencyGraph.h>

using namespace LangCore;

static ModuleMetadata makeModule(const std::string &pkgId, const std::string &modId, const std::string &version,
                                 int level, const std::string &type = "g2p") {
    ModuleMetadata m;
    m.packageId = pkgId;
    m.moduleId = modId;
    m.version = version;
    m.level = level;
    m.type = type;
    return m;
}

static ResolvedDependency makeResDep(const std::string &pkgId, const std::string &modId, const std::string &version,
                                     int level) {
    ResolvedDependency rd;
    rd.packageId = pkgId;
    rd.moduleId = modId;
    rd.version = version;
    rd.level = level;
    return rd;
}

TEST_CASE(Graph_DiamondDependency_Topology) {
    DependencyGraph graph;
    auto modA = makeModule("pkg", "modA", "1.0.0", 1);
    auto modB = makeModule("pkg", "modB", "1.0.0", 1);
    auto modC = makeModule("pkg", "modC", "1.0.0", 1);
    auto modD = makeModule("pkg", "modD", "1.0.0", 1);

    modA.resolvedDependencies.push_back(makeResDep("pkg", "modB", "1.0.0", 1));
    modA.resolvedDependencies.push_back(makeResDep("pkg", "modC", "1.0.0", 1));
    modB.resolvedDependencies.push_back(makeResDep("pkg", "modD", "1.0.0", 1));
    modC.resolvedDependencies.push_back(makeResDep("pkg", "modD", "1.0.0", 1));

    graph.addModule(modA);
    graph.addModule(modB);
    graph.addModule(modC);
    graph.addModule(modD);
    graph.buildGraph();

    auto plans = graph.getPackageInitializationOrder();
    ASSERT_EQ(plans.size(), 1u);
    auto &order = plans[0].initializationOrder;
    ASSERT_EQ(order.size(), 4u);
    ASSERT_STREQ(order[0].moduleId.c_str(), "modD");
    ASSERT_STREQ(order[3].moduleId.c_str(), "modA");
}

TEST_CASE(Graph_CrossPackage_DiamondDependency) {
    DependencyGraph graph;
    auto main = makeModule("pkg-main", "main", "1.0.0", 1);
    auto depA = makeModule("pkg-a", "depA", "1.0.0", 1);
    auto depB = makeModule("pkg-b", "depB", "1.0.0", 1);
    auto common = makeModule("pkg-common", "common", "1.0.0", 1);

    main.resolvedDependencies.push_back(makeResDep("pkg-a", "depA", "1.0.0", 1));
    main.resolvedDependencies.push_back(makeResDep("pkg-b", "depB", "1.0.0", 1));
    depA.resolvedDependencies.push_back(makeResDep("pkg-common", "common", "1.0.0", 1));
    depB.resolvedDependencies.push_back(makeResDep("pkg-common", "common", "1.0.0", 1));

    graph.addModule(main);
    graph.addModule(depA);
    graph.addModule(depB);
    graph.addModule(common);
    graph.buildGraph();

    auto plans = graph.getPackageInitializationOrder();
    ASSERT_EQ(plans.size(), 4u);
    ASSERT_STREQ(plans[0].packageId.c_str(), "pkg-common");
}

TEST_CASE(Graph_FourNode_TransitiveDependency) {
    DependencyGraph graph;
    auto modA = makeModule("pkg", "modA", "1.0.0", 1);
    auto modB = makeModule("pkg", "modB", "1.0.0", 1);
    auto modC = makeModule("pkg", "modC", "1.0.0", 1);
    auto modD = makeModule("pkg", "modD", "1.0.0", 1);

    modA.resolvedDependencies.push_back(makeResDep("pkg", "modB", "1.0.0", 1));
    modB.resolvedDependencies.push_back(makeResDep("pkg", "modC", "1.0.0", 1));
    modC.resolvedDependencies.push_back(makeResDep("pkg", "modD", "1.0.0", 1));

    graph.addModule(modA);
    graph.addModule(modB);
    graph.addModule(modC);
    graph.addModule(modD);
    graph.buildGraph();

    auto plans = graph.getPackageInitializationOrder();
    ASSERT_EQ(plans.size(), 1u);
    auto &order = plans[0].initializationOrder;
    ASSERT_EQ(order.size(), 4u);
    ASSERT_STREQ(order[0].moduleId.c_str(), "modD");
    ASSERT_STREQ(order[3].moduleId.c_str(), "modA");
}

TEST_CASE(Graph_EmptyGraph_InitOrder) {
    DependencyGraph graph;
    graph.buildGraph();
    auto plans = graph.getPackageInitializationOrder();
    ASSERT_EQ(plans.size(), 0u);
}

TEST_CASE(Graph_SelfDependency_SamePackage) {
    DependencyGraph graph;
    auto modA = makeModule("pkg", "modA", "1.0.0", 1);
    auto modB = makeModule("pkg", "modB", "1.0.0", 1);
    modA.resolvedDependencies.push_back(makeResDep("pkg", "modB", "1.0.0", 1));

    graph.addModule(modA);
    graph.addModule(modB);
    graph.buildGraph();

    auto cycles = graph.findCycles();
    ASSERT_EQ(cycles.size(), 0u);
}

TEST_CASE(Graph_SingleNode_NoDeps) {
    DependencyGraph graph;
    graph.addModule(makeModule("pkg", "standalone", "1.0.0", 1));
    graph.buildGraph();

    auto plans = graph.getPackageInitializationOrder();
    ASSERT_EQ(plans.size(), 1u);
    ASSERT_EQ(plans[0].initializationOrder.size(), 1u);
}

TEST_CASE(Graph_ThreePackage_Transitive) {
    DependencyGraph graph;
    auto modA = makeModule("pkgA", "modA", "1.0.0", 1);
    auto modB = makeModule("pkgB", "modB", "1.0.0", 1);
    auto modC = makeModule("pkgC", "modC", "1.0.0", 1);
    modA.resolvedDependencies.push_back(makeResDep("pkgB", "modB", "1.0.0", 1));
    modB.resolvedDependencies.push_back(makeResDep("pkgC", "modC", "1.0.0", 1));

    graph.addModule(modA);
    graph.addModule(modB);
    graph.addModule(modC);
    graph.buildGraph();

    auto plans = graph.getPackageInitializationOrder();
    ASSERT_EQ(plans.size(), 3u);
    ASSERT_STREQ(plans[0].packageId.c_str(), "pkgC");
    ASSERT_STREQ(plans[1].packageId.c_str(), "pkgB");
    ASSERT_STREQ(plans[2].packageId.c_str(), "pkgA");
}

TEST_CASE(Graph_Cyclic_DirectMutual) {
    DependencyGraph graph;
    auto modA = makeModule("pkg", "modA", "1.0.0", 1);
    auto modB = makeModule("pkg", "modB", "1.0.0", 1);
    modA.resolvedDependencies.push_back(makeResDep("pkg", "modB", "1.0.0", 1));
    modB.resolvedDependencies.push_back(makeResDep("pkg", "modA", "1.0.0", 1));

    graph.addModule(modA);
    graph.addModule(modB);
    graph.buildGraph();

    auto cycles = graph.findCycles();
    ASSERT_GT(cycles.size(), 0u);
}

TEST_CASE(Graph_Cyclic_ThreeNode) {
    DependencyGraph graph;
    auto modA = makeModule("pkg", "modA", "1.0.0", 1);
    auto modB = makeModule("pkg", "modB", "1.0.0", 1);
    auto modC = makeModule("pkg", "modC", "1.0.0", 1);
    modA.resolvedDependencies.push_back(makeResDep("pkg", "modB", "1.0.0", 1));
    modB.resolvedDependencies.push_back(makeResDep("pkg", "modC", "1.0.0", 1));
    modC.resolvedDependencies.push_back(makeResDep("pkg", "modA", "1.0.0", 1));

    graph.addModule(modA);
    graph.addModule(modB);
    graph.addModule(modC);
    graph.buildGraph();

    auto cycles = graph.findCycles();
    ASSERT_GT(cycles.size(), 0u);
}

TEST_CASE(Graph_NoCycles_DAG) {
    DependencyGraph graph;
    auto modA = makeModule("pkg", "modA", "1.0.0", 1);
    auto modB = makeModule("pkg", "modB", "1.0.0", 1);
    auto modC = makeModule("pkg", "modC", "1.0.0", 1);
    modA.resolvedDependencies.push_back(makeResDep("pkg", "modB", "1.0.0", 1));
    modA.resolvedDependencies.push_back(makeResDep("pkg", "modC", "1.0.0", 1));

    graph.addModule(modA);
    graph.addModule(modB);
    graph.addModule(modC);
    graph.buildGraph();

    auto cycles = graph.findCycles();
    ASSERT_EQ(cycles.size(), 0u);
}

TEST_CASE(Graph_BuildGraph_NoDepsModule) {
    DependencyGraph graph;
    graph.addModule(makeModule("pkg", "modA", "1.0.0", 1));
    graph.addModule(makeModule("pkg", "modB", "1.0.0", 1));
    ASSERT_TRUE(graph.buildGraph());
}

TEST_CASE(Graph_InitOrder_ModuleInMiddle) {
    DependencyGraph graph;
    auto modA = makeModule("pkg", "modA", "1.0.0", 1);
    auto modB = makeModule("pkg", "modB", "1.0.0", 1);
    auto modC = makeModule("pkg", "modC", "1.0.0", 1);

    modA.resolvedDependencies.push_back(makeResDep("pkg", "modB", "1.0.0", 1));
    modB.resolvedDependencies.push_back(makeResDep("pkg", "modC", "1.0.0", 1));

    graph.addModule(modA);
    graph.addModule(modB);
    graph.addModule(modC);
    graph.buildGraph();

    auto plans = graph.getPackageInitializationOrder();
    auto &order = plans[0].initializationOrder;
    ASSERT_EQ(order.size(), 3u);
    ASSERT_STREQ(order[0].moduleId.c_str(), "modC");
    ASSERT_STREQ(order[1].moduleId.c_str(), "modB");
    ASSERT_STREQ(order[2].moduleId.c_str(), "modA");
}