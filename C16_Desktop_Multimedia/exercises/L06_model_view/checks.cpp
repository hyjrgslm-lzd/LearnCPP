#include <solution.hpp>

#include <QAbstractItemModelTester>
#include <QCoreApplication>
#include <QItemSelectionModel>
#include <QPersistentModelIndex>
#include <QSortFilterProxyModel>
#include <QTest>

#include <iostream>

class ModelContractTest : public QObject {
    Q_OBJECT
private slots:
    void rolesAndInsertion()
    {
        c16_l06::MediaListModel model;
        QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::QtTest);
        model.addClip({"a", "alpha", 30});
        model.addClip({"c", "camera", 10});
        QVERIFY(model.insertClip(1, {"b", "beta", 20}));
        QCOMPARE(model.rowCount({}), 3);
        auto middle = model.index(1, 0);
        QCOMPARE(model.data(middle, c16_l06::MediaListModel::IdRole).toString(), QString("b"));
        QCOMPARE(model.data(middle, c16_l06::MediaListModel::TitleRole).toString(), QString("beta"));
        QCOMPARE(model.data(middle, c16_l06::MediaListModel::DurationRole).toInt(), 20);
    }

    void moveKeepsIdentityAndSelection()
    {
        c16_l06::MediaListModel model;
        QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::QtTest);
        model.addClip({"a", "alpha", 30});
        model.addClip({"b", "beta", 20});
        model.addClip({"c", "camera", 10});
        QPersistentModelIndex b_index(model.index(1, 0));
        QVERIFY(model.moveClip("b", 0));
        QVERIFY(b_index.isValid());
        QCOMPARE(model.data(b_index, c16_l06::MediaListModel::IdRole).toString(), QString("b"));
        QVERIFY(model.renameClip("b", "bravo"));
        QCOMPARE(model.data(b_index, c16_l06::MediaListModel::TitleRole).toString(), QString("bravo"));

        QSortFilterProxyModel proxy;
        proxy.setSourceModel(&model);
        proxy.setFilterRole(c16_l06::MediaListModel::TitleRole);
        proxy.setFilterFixedString("bravo");
        QCOMPARE(proxy.rowCount({}), 1);
        QItemSelectionModel selection(&proxy);
        selection.select(proxy.index(0, 0), QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        QCOMPARE(selection.selectedRows().size(), 1);
        auto source = proxy.mapToSource(selection.selectedRows().front());
        QCOMPARE(model.data(source, c16_l06::MediaListModel::IdRole).toString(), QString("b"));
    }

    void removalInvalidatesPersistentIndex()
    {
        c16_l06::MediaListModel model;
        QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::QtTest);
        model.addClip({"a", "alpha", 30});
        model.addClip({"b", "beta", 20});
        QPersistentModelIndex removed(model.index(1, 0));
        QVERIFY(model.removeClip("b"));
        QVERIFY(!removed.isValid());
        QCOMPARE(model.rowCount({}), 1);
        QVERIFY(!model.removeClip("missing"));
    }
};

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    ModelContractTest test;
    const int result = QTest::qExec(&test, argc, argv);
    if (result != 0) std::cerr << "FAIL!\n";
    return result == 0 ? 0 : 1;
}

#include "checks.moc"
