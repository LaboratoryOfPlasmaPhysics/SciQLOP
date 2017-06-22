#ifndef SCIQLOP_GENERATEVARIABLEMENUOPERATION_H
#define SCIQLOP_GENERATEVARIABLEMENUOPERATION_H

#include "Visualization/IVisualizationWidgetVisitor.h"

#include <Common/spimpl.h>

class QMenu;
class Variable;

/**
 * @brief The GenerateVariableMenuOperation class defines an operation that traverses all of
 * visualization widgets to determine which can accommodate a variable. The result of the operation
 * is a menu that contains actions to add the variable into the containers.
 */
class GenerateVariableMenuOperation : public IVisualizationWidgetVisitor {
public:
    /**
     * Ctor
     * @param menu the menu to which to attach the generated menu
     * @param variable the variable for which to generate the menu
     */
    explicit GenerateVariableMenuOperation(QMenu *menu, std::shared_ptr<Variable> variable);

    void visitEnter(VisualizationWidget *widget) override final;
    void visitLeave(VisualizationWidget *widget) override final;
    void visitEnter(VisualizationTabWidget *tabWidget) override final;
    void visitLeave(VisualizationTabWidget *tabWidget) override final;
    void visitEnter(VisualizationZoneWidget *zoneWidget) override final;
    void visitLeave(VisualizationZoneWidget *zoneWidget) override final;
    void visit(VisualizationGraphWidget *graphWidget) override final;

private:
    class GenerateVariableMenuOperationPrivate;
    spimpl::unique_impl_ptr<GenerateVariableMenuOperationPrivate> impl;
};

#endif // SCIQLOP_GENERATEVARIABLEMENUOPERATION_H
