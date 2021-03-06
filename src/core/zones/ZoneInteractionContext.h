// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_ZONES_ZONEINTERACTIONCONTEXT_H_
#define SCANTAILOR_ZONES_ZONEINTERACTIONCONTEXT_H_

#include <boost/function.hpp>

#include "EditableSpline.h"
#include "EditableZoneSet.h"
#include "SplineVertex.h"
#include "ZoneCreationMode.h"

class InteractionHandler;
class InteractionState;
class ImageViewBase;
class EditableZoneSet;

class ZoneInteractionContext {
 public:
  using DefaultInteractionCreator = boost::function<InteractionHandler*()>;

  using ZoneCreationInteractionCreator = boost::function<InteractionHandler*(InteractionState& interaction)>;

  using VertexDragInteractionCreator = boost::function<InteractionHandler*(InteractionState& interaction,
                                                                           const EditableSpline::Ptr& spline,
                                                                           const SplineVertex::Ptr& vertex)>;

  using ZoneDragInteractionCreator
      = boost::function<InteractionHandler*(InteractionState& interaction, const EditableSpline::Ptr& spline)>;

  using ContextMenuInteractionCreator = boost::function<InteractionHandler*(InteractionState& interaction)>;

  using ShowPropertiesCommand = boost::function<void(const EditableZoneSet::Zone& zone)>;

  ZoneInteractionContext(ImageViewBase& imageView, EditableZoneSet& zones);

  virtual ~ZoneInteractionContext();

  ImageViewBase& imageView() { return m_imageView; }

  EditableZoneSet& zones() { return m_zones; }

  virtual InteractionHandler* createDefaultInteraction() { return m_defaultInteractionCreator(); }

  void setDefaultInteractionCreator(const DefaultInteractionCreator& creator) { m_defaultInteractionCreator = creator; }

  virtual InteractionHandler* createZoneCreationInteraction(InteractionState& interaction) {
    return m_zoneCreationInteractionCreator(interaction);
  }

  void setZoneCreationInteractionCreator(const ZoneCreationInteractionCreator& creator) {
    m_zoneCreationInteractionCreator = creator;
  }

  virtual InteractionHandler* createVertexDragInteraction(InteractionState& interaction,
                                                          const EditableSpline::Ptr& spline,
                                                          const SplineVertex::Ptr& vertex) {
    return m_vertexDragInteractionCreator(interaction, spline, vertex);
  }

  void setVertexDragInteractionCreator(const VertexDragInteractionCreator& creator) {
    m_vertexDragInteractionCreator = creator;
  }

  virtual InteractionHandler* createZoneDragInteraction(InteractionState& interaction,
                                                        const EditableSpline::Ptr& spline) {
    return m_zoneDragInteractionCreator(interaction, spline);
  }

  void setZoneDragInteractionCreator(const ZoneDragInteractionCreator& creator) {
    m_zoneDragInteractionCreator = creator;
  }

  /**
   * \note This function may refuse to create a context menu interaction by returning null.
   */
  virtual InteractionHandler* createContextMenuInteraction(InteractionState& interaction) {
    return m_contextMenuInteractionCreator(interaction);
  }

  void setContextMenuInteractionCreator(const ContextMenuInteractionCreator& creator) {
    m_contextMenuInteractionCreator = creator;
  }

  virtual void showPropertiesCommand(const EditableZoneSet::Zone& zone) { m_showPropertiesCommand(zone); }

  void setShowPropertiesCommand(const ShowPropertiesCommand& command) { m_showPropertiesCommand = command; }

  ZoneCreationMode getZoneCreationMode() const { return m_zoneCreationMode; }

  void setZoneCreationMode(ZoneCreationMode zoneCreationMode) { m_zoneCreationMode = zoneCreationMode; }

 private:
  /**
   * Creates an instance of ZoneDefaultInteraction.
   */
  InteractionHandler* createStdDefaultInteraction();

  /**
   * Creates an instance of ZoneCreationInteraction.
   */
  InteractionHandler* createStdZoneCreationInteraction(InteractionState& interaction);

  /**
   * Creates an instance of ZoneVertexDragInteraction.
   */
  InteractionHandler* createStdVertexDragInteraction(InteractionState& interaction,
                                                     const EditableSpline::Ptr& spline,
                                                     const SplineVertex::Ptr& vertex);

  /**
   * Creates an instance of ZoneDragInteraction.
   */
  InteractionHandler* createStdZoneDragInteraction(InteractionState& interaction, const EditableSpline::Ptr& spline);

  /**
   * Creates an instance of ZoneContextMenuInteraction.  May return null.
   */
  InteractionHandler* createStdContextMenuInteraction(InteractionState& interaction);

  static void showPropertiesStub(const EditableZoneSet::Zone&) {}

  ImageViewBase& m_imageView;
  EditableZoneSet& m_zones;
  DefaultInteractionCreator m_defaultInteractionCreator;
  ZoneCreationInteractionCreator m_zoneCreationInteractionCreator;
  VertexDragInteractionCreator m_vertexDragInteractionCreator;
  ZoneDragInteractionCreator m_zoneDragInteractionCreator;
  ContextMenuInteractionCreator m_contextMenuInteractionCreator;
  ShowPropertiesCommand m_showPropertiesCommand;
  ZoneCreationMode m_zoneCreationMode;
};


#endif  // ifndef SCANTAILOR_ZONES_ZONEINTERACTIONCONTEXT_H_
