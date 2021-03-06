// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "Settings.h"

#include <cassert>

#include "AbstractRelinker.h"
#include "RelinkablePath.h"

namespace page_split {
Settings::Settings() : m_defaultLayoutType(AUTO_LAYOUT_TYPE) {}

Settings::~Settings() = default;

void Settings::clear() {
  QMutexLocker locker(&m_mutex);

  m_perPageRecords.clear();
  m_defaultLayoutType = AUTO_LAYOUT_TYPE;
}

void Settings::performRelinking(const AbstractRelinker& relinker) {
  QMutexLocker locker(&m_mutex);
  PerPageRecords newRecords;

  for (const PerPageRecords::value_type& kv : m_perPageRecords) {
    const RelinkablePath oldPath(kv.first.filePath(), RelinkablePath::File);
    ImageId newImageId(kv.first);
    newImageId.setFilePath(relinker.substitutionPathFor(oldPath));
    newRecords.insert(PerPageRecords::value_type(newImageId, kv.second));
  }

  m_perPageRecords.swap(newRecords);
}

LayoutType Settings::defaultLayoutType() const {
  QMutexLocker locker(&m_mutex);
  return m_defaultLayoutType;
}

void Settings::setLayoutTypeForAllPages(const LayoutType layoutType) {
  QMutexLocker locker(&m_mutex);

  auto it(m_perPageRecords.begin());
  const auto end(m_perPageRecords.end());
  while (it != end) {
    if (it->second.hasLayoutTypeConflict(layoutType)) {
      m_perPageRecords.erase(it++);
    } else {
      it->second.clearLayoutType();
      ++it;
    }
  }

  m_defaultLayoutType = layoutType;
}

void Settings::setLayoutTypeFor(const LayoutType layoutType, const std::set<PageId>& pages) {
  QMutexLocker locker(&m_mutex);

  UpdateAction action;

  for (const PageId& pageId : pages) {
    updatePageLocked(pageId.imageId(), action);
  }
}

Settings::Record Settings::getPageRecord(const ImageId& imageId) const {
  QMutexLocker locker(&m_mutex);
  return getPageRecordLocked(imageId);
}

Settings::Record Settings::getPageRecordLocked(const ImageId& imageId) const {
  auto it(m_perPageRecords.find(imageId));
  if (it == m_perPageRecords.end()) {
    return Record(m_defaultLayoutType);
  } else {
    return Record(it->second, m_defaultLayoutType);
  }
}

void Settings::updatePage(const ImageId& imageId, const UpdateAction& action) {
  QMutexLocker locker(&m_mutex);
  updatePageLocked(imageId, action);
}

void Settings::updatePageLocked(const ImageId& imageId, const UpdateAction& action) {
  auto it(m_perPageRecords.find(imageId));
  if (it == m_perPageRecords.end()) {
    // No record exists for this page.

    Record record(m_defaultLayoutType);
    record.update(action);

    if (record.hasLayoutTypeConflict()) {
      record.clearParams();
    }

    if (!record.isNull()) {
      m_perPageRecords.insert(it, PerPageRecords::value_type(imageId, record));
    }
  } else {
    // A record was found.
    updatePageLocked(it, action);
  }
}

void Settings::updatePageLocked(const PerPageRecords::iterator it, const UpdateAction& action) {
  Record record(it->second, m_defaultLayoutType);
  record.update(action);

  if (record.hasLayoutTypeConflict()) {
    record.clearParams();
  }

  if (record.isNull()) {
    m_perPageRecords.erase(it);
  } else {
    it->second = record;
  }
}

Settings::Record Settings::conditionalUpdate(const ImageId& imageId, const UpdateAction& action, bool* conflict) {
  QMutexLocker locker(&m_mutex);

  auto it(m_perPageRecords.find(imageId));
  if (it == m_perPageRecords.end()) {
    // No record exists for this page.

    Record record(m_defaultLayoutType);
    record.update(action);

    if (record.hasLayoutTypeConflict()) {
      if (conflict) {
        *conflict = true;
      }
      return Record(m_defaultLayoutType);
    }

    if (!record.isNull()) {
      m_perPageRecords.insert(it, PerPageRecords::value_type(imageId, record));
    }

    if (conflict) {
      *conflict = false;
    }
    return record;
  } else {
    // A record was found.

    Record record(it->second, m_defaultLayoutType);
    record.update(action);

    if (record.hasLayoutTypeConflict()) {
      if (conflict) {
        *conflict = true;
      }
      return Record(it->second, m_defaultLayoutType);
    }

    if (conflict) {
      *conflict = false;
    }

    if (record.isNull()) {
      m_perPageRecords.erase(it);
      return Record(m_defaultLayoutType);
    } else {
      it->second = record;
      return record;
    }
  }
}  // Settings::conditionalUpdate

/*======================= Settings::BaseRecord ======================*/

Settings::BaseRecord::BaseRecord()
    : m_params(PageLayout(), Dependencies(), MODE_AUTO),
      m_layoutType(AUTO_LAYOUT_TYPE),
      m_paramsValid(false),
      m_layoutTypeValid(false) {}

void Settings::BaseRecord::setParams(const Params& params) {
  m_params = params;
  m_paramsValid = true;
}

void Settings::BaseRecord::setLayoutType(const LayoutType layoutType) {
  m_layoutType = layoutType;
  m_layoutTypeValid = true;
}

bool Settings::BaseRecord::hasLayoutTypeConflict(const LayoutType layoutType) const {
  if (!m_paramsValid) {
    // No data - no conflict.
    return false;
  }

  if (layoutType == AUTO_LAYOUT_TYPE) {
    // This one is compatible with everything.
    return false;
  }

  switch (m_params.pageLayout().type()) {
    case PageLayout::SINGLE_PAGE_UNCUT:
      return layoutType != SINGLE_PAGE_UNCUT;
    case PageLayout::SINGLE_PAGE_CUT:
      return layoutType != PAGE_PLUS_OFFCUT;
    case PageLayout::TWO_PAGES:
      return layoutType != TWO_PAGES;
  }

  assert(!"Unreachable");
  return false;
}

const LayoutType* Settings::BaseRecord::layoutType() const {
  return m_layoutTypeValid ? &m_layoutType : nullptr;
}

const Params* Settings::BaseRecord::params() const {
  return m_paramsValid ? &m_params : nullptr;
}

bool Settings::BaseRecord::isNull() const {
  return !(m_paramsValid || m_layoutTypeValid);
}

void Settings::BaseRecord::clearParams() {
  m_paramsValid = false;
}

void Settings::BaseRecord::clearLayoutType() {
  m_layoutTypeValid = false;
}

/*========================= Settings::Record ========================*/

Settings::Record::Record(const LayoutType defaultLayoutType) : m_defaultLayoutType(defaultLayoutType) {}

Settings::Record::Record(const BaseRecord& baseRecord, const LayoutType defaultLayoutType)
    : BaseRecord(baseRecord), m_defaultLayoutType(defaultLayoutType) {}

LayoutType Settings::Record::combinedLayoutType() const {
  return m_layoutTypeValid ? m_layoutType : m_defaultLayoutType;
}

void Settings::Record::update(const UpdateAction& action) {
  switch (action.m_layoutTypeAction) {
    case UpdateAction::SET:
      setLayoutType(action.m_layoutType);
      break;
    case UpdateAction::CLEAR:
      clearLayoutType();
      break;
    case UpdateAction::DONT_TOUCH:
      break;
  }

  switch (action.m_paramsAction) {
    case UpdateAction::SET:
      setParams(action.m_params);
      break;
    case UpdateAction::CLEAR:
      clearParams();
      break;
    case UpdateAction::DONT_TOUCH:
      break;
  }
}

bool Settings::Record::hasLayoutTypeConflict() const {
  return BaseRecord::hasLayoutTypeConflict(combinedLayoutType());
}

/*======================= Settings::UpdateAction ======================*/

Settings::UpdateAction::UpdateAction()
    : m_params(PageLayout(), Dependencies(), MODE_AUTO),
      m_layoutType(AUTO_LAYOUT_TYPE),
      m_paramsAction(DONT_TOUCH),
      m_layoutTypeAction(DONT_TOUCH) {}

void Settings::UpdateAction::setLayoutType(const LayoutType layoutType) {
  m_layoutType = layoutType;
  m_layoutTypeAction = SET;
}

void Settings::UpdateAction::clearLayoutType() {
  m_layoutTypeAction = CLEAR;
}

void Settings::UpdateAction::setParams(const Params& params) {
  m_params = params;
  m_paramsAction = SET;
}

void Settings::UpdateAction::clearParams() {
  m_paramsAction = CLEAR;
}
}  // namespace page_split