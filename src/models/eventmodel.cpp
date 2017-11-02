/*
  eventmodel.cpp

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

#include "eventmodel.h"

#include "../util.h"

#include <QSet>

EventModel::EventModel(QObject* parent)
    : QAbstractTableModel(parent)
{
}

EventModel::~EventModel() = default;

int EventModel::columnCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : NUM_COLUMNS;
}

int EventModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_data.threads.size();
}

QVariant EventModel::headerData(int section, Qt::Orientation orientation,
                                int role) const
{
    if (section < 0 || section >= NUM_COLUMNS || orientation != Qt::Horizontal
        || (role != Qt::DisplayRole && role != Qt::InitialSortOrderRole))
    {
        return {};
    }

    switch (static_cast<Columns>(section)) {
    case ThreadColumn:
        return tr("Thread");
    case EventsColumn:
        return tr("Events");
    case NUM_COLUMNS:
        // nothing
        break;
    }

    return {};
}

QVariant EventModel::data(const QModelIndex& index, int role) const
{
    if (!hasIndex(index.row(), index.column(), index.parent())) {
        return {};
    }

    const auto& thread = m_data.threads[index.row()];

    if (role == ThreadStartRole) {
        return thread.timeStart;
    } else if (role == ThreadEndRole) {
        return thread.timeEnd;
    } else if (role == ThreadNameRole) {
        return thread.name;
    } else if (role == ThreadIdRole) {
        return thread.tid;
    } else if (role == ProcessIdRole) {
        return thread.pid;
    } else if (role == MaxTimeRole) {
        return m_maxTime;
    } else if (role == MinTimeRole) {
        return m_minTime;
    } else if (role == EventsRole) {
        return QVariant::fromValue(thread.events);
    } else if (role == MaxCostRole) {
        return m_maxCost;
    } else if (role == NumProcessesRole) {
        return m_numProcesses;
    } else if (role == NumThreadsRole) {
        return m_numThreads;
    } else if (role == SortRole) {
        if (index.column() == ThreadColumn)
            return thread.name;
        else
            return thread.events.size();
    } else if (role == EventTypesRole) {
        return QVariant::fromValue(m_data.eventTypes);
    } else if (role == EventResultsRole) {
        return QVariant::fromValue(m_data);
    }

    switch (static_cast<Columns>(index.column())) {
        case ThreadColumn:
            if (role == Qt::DisplayRole) {
                return tr("%1 (#%2)").arg(thread.name, QString::number(thread.tid));
            } else if (role == Qt::ToolTipRole) {
                return tr("Thread %1 (tid=%2, pid=%3) ran for %4 (%5% of total runtime).\n"
                        "It produced %6 events (%7% of the total events).")
                        .arg(thread.name, QString::number(thread.tid), QString::number(thread.pid),
                             Util::formatTimeString(thread.timeEnd - thread.timeStart),
                             Util::formatCostRelative(thread.timeEnd - thread.timeStart,
                                                      m_maxTime - m_minTime),
                             QString::number(thread.events.size()),
                             Util::formatCostRelative(thread.events.size(), m_totalEvents));
            }
            break;
        case EventsColumn:
            if (role == Qt::DisplayRole)
                return thread.events.size();
            break;
        case NUM_COLUMNS:
            // nothing
            break;
    }

    return {};
}

void EventModel::setData(const Data::EventResults& data)
{
    beginResetModel();
    m_data = data;
    m_totalEvents = 0;
    m_maxCost = 0;
    m_numProcesses = 0;
    m_numThreads = 0;
    if (data.threads.isEmpty()) {
        m_minTime = 0;
        m_maxTime = 0;
    } else {
        m_minTime = data.threads.first().timeStart;
        m_maxTime = data.threads.first().timeEnd;
        QSet<quint32> processes;
        QSet<quint32> threads;
        for (const auto& thread : data.threads) {
            m_minTime = std::min(thread.timeStart, m_minTime);
            m_maxTime = std::max(thread.timeEnd, m_maxTime);
            m_totalEvents += thread.events.size();
            processes.insert(thread.pid);
            threads.insert(thread.tid);

            for (const auto& event : thread.events) {
                if (event.type != 0) {
                    // TODO: support multiple cost types somehow
                    continue;
                }
                m_maxCost = std::max(event.cost, m_maxCost);
            }
        }
        m_numProcesses = processes.size();
        m_numThreads = threads.size();
    }
    endResetModel();
}
