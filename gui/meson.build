
gui_moc_headers = [
 './include/Common/VisualizationDef.h',
 './include/Common/ColorUtils.h',
 './include/DragAndDrop/DragDropGuiController.h',
 './include/DragAndDrop/DragDropTabSwitcher.h',
 './include/DragAndDrop/DragDropScroller.h',
 './include/Settings/SqpSettingsDialog.h',
 './include/Settings/SqpSettingsGeneralWidget.h',
 './include/DataSource/DataSourceTreeWidgetHelper.h',
 './include/DataSource/DataSourceTreeWidget.h',
 './include/DataSource/DataSourceTreeWidgetItem.h',
 './include/DataSource/DataSourceWidget.h',
 './include/Catalogue2/repositoriestreeview.h',
 './include/Catalogue2/browser.h',
 './include/Catalogue2/eventeditor.h',
 './include/Catalogue2/eventsmodel.h',
 './include/Catalogue2/eventstreeview.h',
 './include/Catalogue2/repositoriesmodel.h',
 './include/TimeWidget/TimeWidget.h',
 './include/SqpApplication.h',
 './include/SidePane/SqpSidePane.h',
 './include/Variable/RenameVariableDialog.h',
 './include/Variable/VariableInspectorWidget.h',
 './include/Variable/VariableInspectorTableView.h',
 './include/Variable/VariableMenuHeaderWidget.h',
 './include/Visualization/VisualizationDragWidget.h',
 './include/Visualization/VisualizationZoneWidget.h',
 './include/Visualization/operations/GenerateVariableMenuOperation.h',
 './include/Visualization/operations/RescaleAxeOperation.h',
 './include/Visualization/operations/RemoveVariableOperation.h',
 './include/Visualization/operations/MenuBuilder.h',
 './include/Visualization/operations/FindVariableOperation.h',
 './include/Visualization/VisualizationDefs.h',
 './include/Visualization/IVisualizationWidgetVisitor.h',
 './include/Visualization/SqpColorScale.h',
 './include/Visualization/VisualizationGraphRenderingDelegate.h',
 './include/Visualization/VisualizationGraphWidget.h',
 './include/Visualization/MacScrollBarStyle.h',
 './include/Visualization/IVisualizationWidget.h',
 './include/Visualization/qcustomplot.h',
 './include/Visualization/IGraphSynchronizer.h',
 './include/Visualization/QCPColorMapIterator.h',
 './include/Visualization/VisualizationActionManager.h',
 './include/Visualization/VisualizationTabWidget.h',
 './include/Visualization/IVariableContainer.h',
 './include/Visualization/AxisRenderingUtils.h',
 './include/Visualization/VisualizationMultiZoneSelectionDialog.h',
 './include/Visualization/VisualizationCursorItem.h',
 './include/Visualization/VisualizationWidget.h',
 './include/Visualization/PlottablesRenderingUtils.h',
 './include/Visualization/VisualizationSelectionZoneManager.h',
 './include/Visualization/QCustomPlotSynchronizer.h',
 './include/Visualization/VisualizationSelectionZoneItem.h',
 './include/Visualization/VisualizationDragDropContainer.h',
 './include/Visualization/ColorScaleEditor.h',
 './include/Visualization/VisualizationGraphHelper.h',
 './include/Actions/ActionsGuiController.h',
 './include/Actions/FilteringAction.h',
 './include/Actions/SelectionZoneAction.h'
 ]


gui_ui_files = [
 './ui/Settings/SqpSettingsGeneralWidget.ui',
 './ui/Settings/SqpSettingsDialog.ui',
 './ui/DataSource/DataSourceWidget.ui',
 './ui/Catalogue2/browser.ui',
 './ui/Catalogue2/eventeditor.ui',
 './ui/TimeWidget/TimeWidget.ui',
 './ui/SidePane/SqpSidePane.ui',
 './ui/Variable/RenameVariableDialog.ui',
 './ui/Variable/VariableInspectorWidget.ui',
 './ui/Variable/VariableMenuHeaderWidget.ui',
 './ui/Visualization/ColorScaleEditor.ui',
 './ui/Visualization/VisualizationZoneWidget.ui',
 './ui/Visualization/VisualizationMultiZoneSelectionDialog.ui',
 './ui/Visualization/VisualizationGraphWidget.ui',
 './ui/Visualization/VisualizationWidget.ui',
 './ui/Visualization/VisualizationTabWidget.ui'
]

gui_qresources = ['resources/sqpguiresources.qrc']

rcc_gen = generator(rcc,
                output  : 'qrc_@BASENAME@.cpp',
                arguments : [
                 '--output',
                 '@OUTPUT@',
                 '@INPUT@',
                 '@EXTRA_ARGS@'])

rcc_files = rcc_gen.process(gui_qresources, extra_args : ['-name', 'sqpguiresources'])

gui_moc_files = qt5.preprocess(moc_headers : gui_moc_headers,
                               ui_files : gui_ui_files)

gui_sources = [
 './src/Common/ColorUtils.cpp',
 './src/Common/VisualizationDef.cpp',
 './src/SqpApplication.cpp',
 './src/DragAndDrop/DragDropTabSwitcher.cpp',
 './src/DragAndDrop/DragDropScroller.cpp',
 './src/DragAndDrop/DragDropGuiController.cpp',
 './src/Settings/SqpSettingsGeneralWidget.cpp',
 './src/Settings/SqpSettingsDialog.cpp',
 './src/DataSource/DataSourceTreeWidgetItem.cpp',
 './src/DataSource/DataSourceTreeWidgetHelper.cpp',
 './src/DataSource/DataSourceWidget.cpp',
 './src/DataSource/DataSourceTreeWidget.cpp',
 './src/Catalogue2/eventstreeview.cpp',
 './src/Catalogue2/eventeditor.cpp',
 './src/Catalogue2/repositoriestreeview.cpp',
 './src/Catalogue2/browser.cpp',
 './src/Catalogue2/eventsmodel.cpp',
 './src/Catalogue2/repositoriesmodel.cpp',
 './src/TimeWidget/TimeWidget.cpp',
 './src/SidePane/SqpSidePane.cpp',
 './src/Variable/VariableInspectorTableView.cpp',
 './src/Variable/VariableInspectorWidget.cpp',
 './src/Variable/RenameVariableDialog.cpp',
 './src/Variable/VariableMenuHeaderWidget.cpp',
 './src/Visualization/VisualizationGraphWidget.cpp',
 './src/Visualization/PlottablesRenderingUtils.cpp',
 './src/Visualization/AxisRenderingUtils.cpp',
 './src/Visualization/VisualizationWidget.cpp',
 './src/Visualization/qcustomplot.cpp',
 './src/Visualization/VisualizationDragWidget.cpp',
 './src/Visualization/VisualizationActionManager.cpp',
 './src/Visualization/MacScrollBarStyle.cpp',
 './src/Visualization/VisualizationSelectionZoneManager.cpp',
 './src/Visualization/operations/FindVariableOperation.cpp',
 './src/Visualization/operations/RescaleAxeOperation.cpp',
 './src/Visualization/operations/MenuBuilder.cpp',
 './src/Visualization/operations/GenerateVariableMenuOperation.cpp',
 './src/Visualization/operations/RemoveVariableOperation.cpp',
 './src/Visualization/VisualizationSelectionZoneItem.cpp',
 './src/Visualization/VisualizationCursorItem.cpp',
 './src/Visualization/QCPColorMapIterator.cpp',
 './src/Visualization/QCustomPlotSynchronizer.cpp',
 './src/Visualization/ColorScaleEditor.cpp',
 './src/Visualization/VisualizationMultiZoneSelectionDialog.cpp',
 './src/Visualization/VisualizationTabWidget.cpp',
 './src/Visualization/VisualizationGraphHelper.cpp',
 './src/Visualization/VisualizationGraphRenderingDelegate.cpp',
 './src/Visualization/VisualizationDragDropContainer.cpp',
 './src/Visualization/VisualizationZoneWidget.cpp',
 './src/Visualization/SqpColorScale.cpp',
 './src/Actions/FilteringAction.cpp',
 './src/Actions/SelectionZoneAction.cpp',
 './src/Actions/ActionsGuiController.cpp'
 ]

gui_inc = include_directories(['include', 'include/Visualization'])

sciqlop_gui_lib = library('sciqlopgui',
                       gui_sources,
                       gui_moc_files,
                       rcc_files,
                       include_directories : [gui_inc],
                       dependencies : [ qt5printsupport, qt5gui, qt5widgets, qt5svg, sciqlop_core],
                       install : true
                       )

sciqlop_gui = declare_dependency(link_with : sciqlop_gui_lib,
                                include_directories : gui_inc,
                                dependencies : [qt5printsupport, qt5gui, qt5widgets, qt5svg, sciqlop_core])

gui_tests_inc = include_directories(['tests/GUITestUtils'])

catalogue_browser_moc_files = qt5.preprocess(moc_sources : 'tests/catalogue/browser/main.cpp')
catalogue_browser = executable('catalogue_browser', 'tests/catalogue/browser/main.cpp',catalogue_browser_moc_files,
                                include_directories : gui_tests_inc,
                                dependencies :[sciqlop_gui, qt5test])
