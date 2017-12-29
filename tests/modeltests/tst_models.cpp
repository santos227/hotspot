/*
  tst_models.cpp

  This file is part of Hotspot, the Qt GUI for performance analysis.

  Copyright (C) 2017 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Milian Wolff <milian.wolff@kdab.com>

  Licensees holding valid commercial KDAB Hotspot licenses may use this file in
  accordance with Hotspot Commercial License Agreement provided with the Software.

  Contact info@kdab.com if any conditions of this licensing are not clear to you.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <QObject>
#include <QTest>
#include <QDebug>
#include <QTextStream>

#include "modeltest.h"

#include "../testutils.h"

namespace {
Data::BottomUpResults buildBottomUpTree(const QByteArray& stacks)
{
    Data::BottomUpResults ret;
    ret.costs.addType(0, "samples", Data::Costs::Unit::Unknown);
    ret.root.symbol = {"<root>", {}};
    const auto& lines = stacks.split('\n');
    QHash<quint32, Data::Symbol> ids;
    quint32 maxId = 0;
    for (const auto& line : lines) {
        auto trimmed = line.trimmed();
        if (trimmed.isEmpty()) {
            continue;
        }
        const auto& frames = trimmed.split(';');
        auto* parent = &ret.root;
        for (auto it = frames.rbegin(), end = frames.rend(); it != end; ++it) {
            const auto& frame = *it;
            const auto symbol = Data::Symbol{frame, {}};
            auto node = parent->entryForSymbol(symbol, &maxId);
            Q_ASSERT(!ids.contains(node->id) || ids[node->id] == symbol);
            ids[node->id] = symbol;
            ret.costs.increment(0, node->id);
            parent = node;
        }
        ret.costs.incrementTotal(0);
    }
    Data::BottomUp::initializeParents(&ret.root);
    return ret;
}

Data::BottomUpResults generateTree1()
{
    return buildBottomUpTree(R"(
        A;B;C
        A;B;D
        A;B;D
        A;B;C;E
        A;B;C;E;C
        A;B;C;E;C;E
        A;B;C;C
        C
        C
    )");
}

}

class TestModels : public QObject
{
    Q_OBJECT
private slots:
    void testTreeParents()
    {
        const auto tree = generateTree1();

        QVERIFY(!tree.root.parent);
        for (const auto& firstLevel : tree.root.children) {
            QVERIFY(!firstLevel.parent);
            for (const auto& secondLevel : firstLevel.children) {
                QCOMPARE(secondLevel.parent, &firstLevel);
            }
        }
    }

    void testBottomUpModel()
    {
        const auto tree = generateTree1();

        QCOMPARE(tree.costs.totalCost(0), qint64(9));

        const QStringList expectedTree = {
            "C=5",
            " B=1",
            "  A=1",
            " E=1",
            "  C=1",
            "   B=1",
            "    A=1",
            " C=1",
            "  B=1",
            "   A=1",
            "D=2",
            " B=2",
            "  A=2",
            "E=2",
            " C=2",
            "  B=1",
            "   A=1",
            "  E=1",
            "   C=1",
            "    B=1",
            "     A=1"
        };
        QCOMPARE(printTree(tree), expectedTree);

        BottomUpModel model;
        ModelTest tester(&model);

        model.setData(tree);
    }

    void testTopDownModel()
    {
        const auto bottomUpTree = generateTree1();
        const auto tree = Data::TopDownResults::fromBottomUp(bottomUpTree);
        QCOMPARE(tree.inclusiveCosts.totalCost(0), qint64(9));
        QCOMPARE(tree.selfCosts.totalCost(0), qint64(9));

        const QStringList expectedTree = {
            "A=s:0,i:7",
            " B=s:0,i:7",
            "  C=s:1,i:5",
            "   E=s:1,i:3",
            "    C=s:1,i:2",
            "     E=s:1,i:1",
            "   C=s:1,i:1",
            "  D=s:2,i:2",
            "C=s:2,i:2"
        };
        QTextStream(stdout) << "Actual:\n" << printTree(tree).join("\n")
                            << "\nExpected:\n" << expectedTree.join("\n") << "\n";
        QCOMPARE(printTree(tree), expectedTree);

        TopDownModel model;
        ModelTest tester(&model);

        model.setData(tree);
    }

    void testTopProxy()
    {
        BottomUpModel model;
        TopProxy proxy;
        ModelTest tester(&proxy);

        const auto data = generateTree1();
        model.setData(data);

        proxy.setSourceModel(&model);
        QCOMPARE(proxy.rowCount(), model.rowCount());
        QCOMPARE(proxy.columnCount(), 3);

        for (auto i = 0, c = proxy.rowCount(); i < c; ++i) {
            auto index = proxy.index(i, 0, {});
            QVERIFY(index.isValid());
            QVERIFY(!proxy.rowCount(index));
        }
    }

    void testCallerCalleeModel()
    {
        const auto tree = generateTree1();

        Data::CallerCalleeResults results;
        Data::callerCalleesFromBottomUpData(tree, &results);
        const QStringList expectedMap = {
            "A=s:0,i:7",
            "A>B=7",
            "B=s:0,i:7",
            "B<A=7",
            "B>C=5",
            "B>D=2",
            "C=s:5,i:7",
            "C<B=5",
            "C<C=1",
            "C<E=2",
            "C>C=1",
            "C>E=3",
            "D=s:2,i:2",
            "D<B=2",
            "E=s:2,i:3",
            "E<C=3",
            "E>C=2",
        };
        QTextStream(stdout) << "Actual:\n" << printMap(results).join("\n")
                            << "\n\nExpected:\n" << expectedMap.join("\n") << "\n";
        QCOMPARE(printMap(results), expectedMap);

        CallerCalleeModel model;
        ModelTest tester(&model);
        model.setResults(results);
        QTextStream(stdout) << "\nActual Model:\n" << printCallerCalleeModel(model).join("\n") << "\n";
        QCOMPARE(printCallerCalleeModel(model), expectedMap);

        for (const auto& entry : results.entries) {
            {
                CallerModel model;
                ModelTest tester(&model);
                model.setResults(entry.callers, results.selfCosts);
            }
            {
                CalleeModel model;
                ModelTest tester(&model);
                model.setResults(entry.callees, results.selfCosts);
            }
            {
                SourceMapModel model;
                ModelTest tester(&model);
                model.setResults(entry.sourceMap, results.selfCosts);
            }
        }
    }
};

QTEST_GUILESS_MAIN(TestModels);

#include "tst_models.moc"
