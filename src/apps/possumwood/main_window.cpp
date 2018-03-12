#include "main_window.h"

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/join.hpp>

#include <QMenuBar>
#include <QAction>
#include <QMessageBox>
#include <QFileDialog>
#include <QDockWidget>
#include <QApplication>
#include <QDesktopWidget>
#include <QVBoxLayout>
#include <QToolBar>
#include <QLabel>

#include <dependency_graph/values.inl>

#include <qt_node_editor/connected_edge.h>

#include <possumwood_sdk/metadata.inl>
#include <possumwood_sdk/node_data.h>
#include <possumwood_sdk/node_data.h>
#include <possumwood_sdk/app.h>
#include <possumwood_sdk/gl.h>

#include "adaptor.h"
#include "config_dialog.h"
#include "actions.h"
#include "grid.h"

namespace {

QAction* makeAction(QString title, std::function<void()> fn, QWidget* parent) {
	QAction* result = new QAction(title, parent);
	QObject::connect(result, &QAction::triggered, [fn](bool) { fn(); });
	return result;
}
}

MainWindow::MainWindow() : QMainWindow() {
	possumwood::App::instance().setMainWindow(this);

	QWidget* centralWidget = new QWidget();
	QVBoxLayout* centralLayout = new QVBoxLayout(centralWidget);
	centralLayout->setContentsMargins(0, 0, 0, 0);
	centralLayout->setSpacing(0);
	setCentralWidget(centralWidget);

	m_viewport = new Viewport();
	centralLayout->addWidget(m_viewport, 1);

	m_timeline = new TimelineWidget();
	centralLayout->addWidget(m_timeline, 0);

	m_properties = new Properties();
	QDockWidget* propDock = new QDockWidget("Properties", this);
	propDock->setObjectName("properties");
	propDock->setWidget(m_properties);
	propDock->toggleViewAction()->setIcon(QIcon(":icons/dock_properties.png"));
	m_properties->setMinimumWidth(420);
	addDockWidget(Qt::RightDockWidgetArea, propDock);

	m_adaptor = new Adaptor(&possumwood::App::instance().graph());
	auto geom = m_adaptor->sizeHint();
	geom.setWidth(QApplication::desktop()->screenGeometry().width() / 2 - 200);
	m_adaptor->setSizeHint(geom);

	QDockWidget* graphDock = new QDockWidget("Graph", this);
	graphDock->setObjectName("graph");
	graphDock->setWidget(m_adaptor);
	graphDock->toggleViewAction()->setIcon(QIcon(":icons/dock_graph.png"));
	addDockWidget(Qt::LeftDockWidgetArea, graphDock);

	QDockWidget* editorDock = new QDockWidget("Editor", this);
	editorDock->setObjectName("editor");
	QLabel* editorWidget = new QLabel("No editable node selected");
	editorWidget->setAlignment(Qt::AlignCenter);
	editorWidget->setMinimumHeight(100);
	editorDock->setWidget(editorWidget);
	editorDock->toggleViewAction()->setIcon(QIcon(":icons/dock_editor.png"));
	addDockWidget(Qt::RightDockWidgetArea, editorDock);

	// connect the selection signal
	connect(&m_adaptor->scene(), &node_editor::GraphScene::selectionChanged,
	        [editorDock, this](const node_editor::GraphScene::Selection& selection) {
		        // convert the selection to the dependency graph selection, to pass to the
		        //   properties dock
		        dependency_graph::Selection out;
		        {
			        auto& index = possumwood::App::instance().index();

			        for(auto& n : selection.nodes)
				        out.addNode(*index[n].graphNode);

			        for(auto& c : selection.connections) {
				        out.addConnection(index[&(c->fromPort().parentNode())].graphNode->port(c->fromPort().index()),
				                          index[&(c->toPort().parentNode())].graphNode->port(c->toPort().index()));
			        }

			        m_properties->show(out);
		        }

		        // instantiate the editor
		        {
			        unsigned editorCounter = 0;
			        for(auto& n : out.nodes())
				        if(n.get().metadata().blindData<possumwood::Metadata*>()->hasEditor())
					        ++editorCounter;

			        if(editorCounter == 0) {
				        QLabel* editorWidget = new QLabel("No editable node selected");
				        editorWidget->setAlignment(Qt::AlignCenter);
				        editorWidget->setMinimumHeight(100);
				        editorDock->setWidget(editorWidget);

				        m_editor.reset();
			        }
			        else if(editorCounter > 1) {
				        QLabel* editorWidget = new QLabel("More than one editable node selected");
				        editorWidget->setAlignment(Qt::AlignCenter);
				        editorWidget->setMinimumHeight(100);
				        editorDock->setWidget(editorWidget);

				        m_editor.reset();
			        }
			        else {
				        for(auto& n : out.nodes()) {
					        possumwood::Metadata* meta = n.get().metadata().blindData<possumwood::Metadata*>();
					        if(meta->hasEditor()) {
						        m_editor = meta->createEditor(n);
						        editorDock->setWidget(m_editor->widget());
					        }
				        }
			        }
		        }
		    });

	m_log = new Log();
	QDockWidget* logDock = new QDockWidget("Log", this);
	logDock->setObjectName("log");
	logDock->setWidget(m_log);
	logDock->toggleViewAction()->setIcon(QIcon(":icons/dock_log.png"));
	m_log->setMinimumWidth(300);
	addDockWidget(Qt::RightDockWidgetArea, logDock);
	connect(m_adaptor, &Adaptor::logged, [this](QIcon icon, const QString& msg) { m_log->addMessage(icon, msg); });

	// create the context click menu
	m_adaptor->setContextMenuPolicy(Qt::CustomContextMenu);

	{
		QMenu* contextMenu = new QMenu(m_adaptor);
		connect(m_adaptor, &Adaptor::customContextMenuRequested, [contextMenu, this](QPoint pos) {
			contextMenu->move(m_adaptor->mapToGlobal(pos));  // to make sure pos() is right, which is
			                                                 // used for placing the new node
			contextMenu->popup(m_adaptor->mapToGlobal(pos));
		});

		{
			m_newNodeMenu = new SearchableMenu("Create");
			m_newNodeMenu->menuAction()->setShortcut(Qt::Key_Tab);
			contextMenu->addMenu(m_newNodeMenu);

			{
				QAction* newNodeAction = new QAction("Create node", m_adaptor);
				newNodeAction->setShortcut(Qt::Key_Tab);
				m_adaptor->addAction(newNodeAction);

				connect(newNodeAction, &QAction::triggered, [this]() {
					if(m_adaptor->underMouse()) {
						m_newNodeMenu->move(QCursor::pos());
						m_newNodeMenu->popup(QCursor::pos());
					}
				});
			}

			// first, create all "folders"
			std::map<std::string, QMenu*> groups;
			for(auto& m : dependency_graph::Metadata::instances()) {
				std::vector<std::string> pieces;
				boost::split(pieces, m.type(), boost::algorithm::is_any_of("/"));
				assert(pieces.size() >= 1);

				for(unsigned a = 1; a < pieces.size(); ++a) {
					const std::string current = boost::join(std::make_pair(pieces.begin(), pieces.begin() + a), "/");

					if(groups.find(current) == groups.end()) {
						QMenu* menu = new QMenu(pieces[a - 1].c_str());
						menu->setIcon(QIcon(":icons/package.png"));

						groups.insert(std::make_pair(current, menu));
					}
				}
			}

			// then create all items
			std::map<std::string, QAction*> items;
			for(auto& m : dependency_graph::Metadata::instances()) {
				std::string itemName = m.type();

				auto it = m.type().rfind('/');
				if(it != std::string::npos)
					itemName = m.type().substr(it + 1);

				QAction* addNode = makeAction(
				    itemName.c_str(),
				    [&m, itemName, contextMenu, this]() {
					    Actions::createNode(m, itemName,
					                        m_adaptor->mapToScene(m_adaptor->mapFromGlobal(m_newNodeMenu->pos())));
					},
				    m_adaptor);
				addNode->setIcon(QIcon(":icons/add-node.png"));

				items.insert(std::make_pair(m.type(), addNode));
			}

			// then, assemble the menu
			for(auto& m : groups) {
				auto it = m.first.rfind('/');
				if(it != std::string::npos) {
					assert(it > 0);
					std::string parentName = m.first.substr(0, it);

					auto menu = groups.find(parentName);
					assert(menu != groups.end());
					menu->second->addMenu(m.second);
				}
			}

			for(auto& i : items) {
				auto it = i.first.rfind('/');
				if(it != std::string::npos) {
					std::string parentName = i.first.substr(0, it);

					auto menu = groups.find(parentName);
					assert(menu != groups.end());
					menu->second->addAction(i.second);
				}
			}

			// finally, populate the m_newNodeMenu
			for(auto& m : groups)
				if(m.first.find('/') == std::string::npos)
					m_newNodeMenu->addMenu(m.second);
			for(auto& i : items)
				if(i.first.find('/') == std::string::npos)
					m_newNodeMenu->addAction(i.second);
		}

		QAction* separator = new QAction(m_adaptor);
		separator->setSeparator(true);
		contextMenu->addAction(separator);

		contextMenu->addAction(m_adaptor->copyAction());
		contextMenu->addAction(m_adaptor->cutAction());
		contextMenu->addAction(m_adaptor->pasteAction());
		contextMenu->addAction(m_adaptor->deleteAction());

		QAction* separator2 = new QAction(m_adaptor);
		separator2->setSeparator(true);
		contextMenu->addAction(separator2);

		contextMenu->addAction(m_adaptor->undoAction());
		contextMenu->addAction(m_adaptor->redoAction());
	}

	// drawing callback
	connect(m_viewport, SIGNAL(render(float)), this, SLOT(draw(float)));
	possumwood::App::instance().graph().onDirty([this]() { m_viewport->update(); });

	// drawable refresh callback
	possumwood::Drawable::onRefreshQueued([this]() { m_viewport->update(); });

	////////////////////
	// window actions
	QAction* newAct = new QAction(QIcon(":icons/filenew.png"), "&New...", this);
	newAct->setShortcuts(QKeySequence::New);
	connect(newAct, &QAction::triggered, [&](bool) {
		QMessageBox::StandardButton res = QMessageBox::question(this, "New file...", "Do you want to clear the scene?");
		if(res == QMessageBox::Yes) {
			m_properties->show({});
			possumwood::App::instance().newFile();
		}
	});

	QAction* openAct = new QAction(QIcon(":icons/fileopen.png"), "&Open...", this);
	openAct->setShortcuts(QKeySequence::Open);
	connect(openAct, &QAction::triggered, [this](bool) {
		QString filename =
		    QFileDialog::getOpenFileName(this, tr("Open File"), possumwood::App::instance().filename().string().c_str(),
		                                 tr("Possumwood files (*.psw)"));

		if(!filename.isEmpty()) {
			try {
				m_properties->show({});
				possumwood::App::instance().loadFile(filename.toStdString());
			}
			catch(std::exception& err) {
				QMessageBox::critical(this, "Error loading file...", "Error loading " + filename + ":\n" + err.what());
			}
			catch(...) {
				QMessageBox::critical(this, "Error loading file...",
				                      "Error loading " + filename + ":\nUnhandled exception thrown during loading.");
			}
		}
	});

	QAction* saveAct = new QAction(QIcon(":icons/filesave.png"), "&Save...", this);
	saveAct->setShortcuts(QKeySequence::Save);
	QAction* saveAsAct = new QAction(QIcon(":icons/filesaveas.png"), "Save &As...", this);
	saveAsAct->setShortcuts(QKeySequence::SaveAs);

	connect(saveAct, &QAction::triggered, [saveAsAct, this](bool) {
		if(possumwood::App::instance().filename().empty())
			saveAsAct->triggered(true);

		else {
			try {
				possumwood::App::instance().saveFile();
			}
			catch(std::exception& err) {
				QMessageBox::critical(this, "Error saving file...",
				                      "Error saving " +
				                          QString(possumwood::App::instance().filename().string().c_str()) + ":\n" +
				                          err.what());
			}
			catch(...) {
				QMessageBox::critical(this, "Error saving file...",
				                      "Error saving " +
				                          QString(possumwood::App::instance().filename().string().c_str()) +
				                          ":\nUnhandled exception thrown during saving.");
			}
		}
	});

	connect(saveAsAct, &QAction::triggered, [saveAct, this](bool) {
		QString filename =
		    QFileDialog::getSaveFileName(this, tr("Save File"), possumwood::App::instance().filename().string().c_str(),
		                                 tr("Possumwood files (*.psw)"));

		if(!filename.isEmpty()) {
			try {
				possumwood::App::instance().saveFile(filename.toStdString());
			}
			catch(std::exception& err) {
				QMessageBox::critical(this, "Error saving file...",
				                      "Error saving " +
				                          QString(possumwood::App::instance().filename().string().c_str()) + ":\n" +
				                          err.what());
			}
			catch(...) {
				QMessageBox::critical(this, "Error saving file...",
				                      "Error saving " +
				                          QString(possumwood::App::instance().filename().string().c_str()) +
				                          ":\nUnhandled exception thrown during saving.");
			}
		}
	});

	QAction* sceneConfigAct = new QAction(QIcon(":icons/settings-scene.png"), "Scene &configuration...", this);
	connect(sceneConfigAct, &QAction::triggered, [this](bool) {
		ConfigDialog dialog(this, possumwood::App::instance().sceneConfig());
		dialog.setWindowTitle("Scene configuration...");
		dialog.exec();
	});

	QAction* quitAct = new QAction(QIcon(":icons/exit.png"), "&Quit", this);
	connect(quitAct, &QAction::triggered, [this](bool) { close(); });

	/////////////////////
	// toolbar
	QToolBar* docksToolbar = addToolBar("Dock widgets toolbar");
	docksToolbar->setObjectName("docks_toolbar");
	docksToolbar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
	docksToolbar->addAction(graphDock->toggleViewAction());
	docksToolbar->addAction(propDock->toggleViewAction());
	docksToolbar->addAction(logDock->toggleViewAction());
	docksToolbar->addAction(editorDock->toggleViewAction());

	////////////////////
	// file menu

	{
		QMenu* fileMenu = menuBar()->addMenu("&File");

		fileMenu->addAction(newAct);
		fileMenu->addAction(openAct);
		fileMenu->addAction(saveAct);
		fileMenu->addAction(saveAsAct);
		fileMenu->addSeparator();
		fileMenu->addAction(sceneConfigAct);
		fileMenu->addSeparator();
		fileMenu->addAction(quitAct);
	}

	///////////////////////////////
	// copy + paste functionality

	{
		QMenu* editMenu = menuBar()->addMenu("&Edit");

		editMenu->addAction(m_adaptor->undoAction());
		editMenu->addAction(m_adaptor->redoAction());

		editMenu->addSeparator();

		editMenu->addAction(m_adaptor->copyAction());
		editMenu->addAction(m_adaptor->cutAction());
		editMenu->addAction(m_adaptor->pasteAction());
		editMenu->addAction(m_adaptor->deleteAction());
	}

	/////////////////////
	// view menu

	{
		QMenu* viewMenu = menuBar()->addMenu("&View");

		viewMenu->addAction(propDock->toggleViewAction());
		viewMenu->addAction(graphDock->toggleViewAction());
		viewMenu->addAction(logDock->toggleViewAction());
		viewMenu->addAction(editorDock->toggleViewAction());
		viewMenu->addSeparator();
		viewMenu->addAction(docksToolbar->toggleViewAction());
	}

	/////////////////////
	// playback menu
	{
		QMenu* playbackMenu = menuBar()->addMenu("&Playback");

		playbackMenu->addAction(m_timeline->playAction());
	}
}

MainWindow::~MainWindow() {
	possumwood::App::instance().graph().clear();
}

void MainWindow::draw(float dt) {
	GL_CHECK_ERR;

	// draw the floor grid
	static std::unique_ptr<possumwood::Grid> s_grid(new possumwood::Grid());
	s_grid->draw(m_viewport->projection(), m_viewport->modelview());

	// draw everything else
	possumwood::Drawable::ViewportState viewport;
	viewport.width = m_viewport->width();
	viewport.height = m_viewport->height();
	viewport.modelview = m_viewport->modelview();
	viewport.projection = m_viewport->projection();

	GL_CHECK_ERR;

	m_adaptor->draw(viewport);

	GL_CHECK_ERR;
}

Adaptor& MainWindow::adaptor() {
	assert(m_adaptor != nullptr);
	return *m_adaptor;
}
