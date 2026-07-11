#pragma once
#include <QSortFilterProxyModel>
#include <QString>

class CustomFilterProxyModel : public QSortFilterProxyModel {
    Q_OBJECT
public:
    explicit CustomFilterProxyModel(QObject* parent = nullptr);

    // Метод для установки активного тега для фильтрации
    void setSelectedTag(const QString& tag);

protected:
    // Главный метод, который решает: показывать строку или скрыть
    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;

private:
    QString m_SelectedTag;
};