/*
  resultsutil.h

  This file is part of Hotspot, the Qt GUI for performance analysis.

  Copyright (C) 2017-2018 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Nate Rogers <nate.rogers@kdab.com>

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

#pragma once

class QTreeView;
class KFilterProxySearchLine;
class QAbstractItemModel;
namespace Data {
class Costs;
struct Symbol;
}

namespace ResultsUtil {
void stretchFirstColumn(QTreeView* view);

void setupTreeView(QTreeView* view, KFilterProxySearchLine* filter,
                   QAbstractItemModel* model, int sortRole, int filterRole,
                   int initialSortColumn);

template<typename Model>
void setupTreeView(QTreeView* view, KFilterProxySearchLine* filter, Model* model)
{
    setupTreeView(view, filter, model, Model::SortRole, Model::FilterRole,
                  Model::InitialSortColumn);
}

void setupCostDelegate(QAbstractItemModel* model, QTreeView* view,
                       int sortRole, int totalCostRole, int numBaseColumns);

template<typename Model>
void setupCostDelegate(Model* model, QTreeView* view)
{
    setupCostDelegate(model, view, Model::SortRole, Model::TotalCostRole,
                      Model::NUM_BASE_COLUMNS);
}

void hideEmptyColumns(const Data::Costs& costs, QTreeView* view, int numBaseColumns);

}
