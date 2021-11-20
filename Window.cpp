#include "Window.hpp"

#include "Application.hpp"
#include "NetworkViewer.hpp"
#include "Optimizer.hpp"

#include <cassert>
#include <exception>
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>

class FunctionalMenuItemEventHandler final : public MenuItemEventHandler {
private:
	std::function<void(MenuItem&)> m_OnClick;

public:
	explicit FunctionalMenuItemEventHandler(std::function<void(MenuItem&)> onClick = nullptr)
		: m_OnClick(std::move(onClick)) {}
	FunctionalMenuItemEventHandler(const FunctionalMenuItemEventHandler&) = delete;
	virtual ~FunctionalMenuItemEventHandler() override = default;

public:
	FunctionalMenuItemEventHandler& operator=(const FunctionalMenuItemEventHandler&) = delete;

public:
	virtual void OnClick(MenuItem& menuItem) override {
		if (m_OnClick) {
			m_OnClick(menuItem);
		}
	}
};

void MainWindowHandler::OnCreate(Control& control) {
	m_Window = &dynamic_cast<Window&>(control);

	m_Window->SetMenu(CreateMenu());

	CreateNewProject();

	m_NetworkViewer = &dynamic_cast<Panel&>(m_Window->AddChild(PanelRef(
		std::make_unique<NetworkViewerHandler>(m_Project->GetNetwork()))));

	m_NetworkViewer->SetSize(m_Window->GetClientSize());
	m_NetworkViewer->Show();
}
void MainWindowHandler::OnClose(Window&, bool& cancel) {
	switch (AskDiscardChanges()) {
	case DialogResult::Yes:
		SaveProject();
		break;

	case DialogResult::No:
		break;

	case DialogResult::Cancel:
		cancel = true;
		break;
	}
}

void MainWindowHandler::OnResize(Control&) {
	if (m_NetworkViewer) {
		m_NetworkViewer->SetSize(m_Window->GetClientSize());
	}
}

MenuRef MainWindowHandler::CreateMenu() {
	MenuRef menu;

	DropDownMenuItemRef project("프로젝트");

	project->AddSubItem(MenuItemRef("새로 만들기", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			switch (AskDiscardChanges()) {
			case DialogResult::Yes:
				SaveProject();
				break;

			case DialogResult::No:
				break;

			case DialogResult::Cancel:
				return;
			}

			CreateNewProject();
		})));
	project->AddSubItem(MenuItemRef("열기", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			switch (AskDiscardChanges()) {
			case DialogResult::Yes:
				SaveProject();
				break;

			case DialogResult::No:
				break;

			case DialogResult::Cancel:
				return;
			}

			OpenFileDialogRef openFileDialog(*m_Window, "열기");

			openFileDialog->AddFilter("프로젝트 파일(*.samp)", "*.samp");
			openFileDialog->AddFilter("모든 파일(*.*)", "*.*");

			if (openFileDialog->Show() != DialogResult::Ok) return;

			try {
				auto newProject = std::make_unique<Project>();

				newProject->Load(openFileDialog->GetPath());

				m_Project = std::move(newProject);
				m_IsSaved = true;

				UpdateText();

				dynamic_cast<NetworkViewerHandler&>(m_NetworkViewer->GetEventHandler()).
					SetTargetNetwork(m_Project->GetNetwork());
			} catch (const std::exception& exception) {
				MessageDialogRef dialog(*m_Window, SAM_APPNAME, "프로젝트를 열지 못했습니다",
					std::string("올바른 ShitAIMaker 프로젝트 파일인지 확인해 보세요. (") + exception.what() + ")",
					MessageDialog::Error, MessageDialog::Ok);

				dialog->Show();
			}
		})));
	project->AddSubItem(MenuItemRef("저장", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			if (!m_IsSaved || m_Project->GetPath().empty()) {
				SaveProject();
			}
		})));
	project->AddSubItem(MenuItemRef("다른 이름으로 저장", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			SaveProject(true);
		})));

	project->AddSubItem(MenuItemSeparatorRef());
	project->AddSubItem(MenuItemRef("끝내기", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			switch (AskDiscardChanges()) {
			case DialogResult::Yes:
				if (SaveProject()) break;
				else return;

			case DialogResult::No:
				break;

			case DialogResult::Cancel:
				return;
			}

			m_Window->Close();
		})));

	menu->AddItem(std::move(project));

	DropDownMenuItemRef network("네트워크");

	network->AddSubItem(MenuItemRef("실행", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			const auto trainData = AskTrainData("실행");

			if (!trainData) return;

			Network& network = m_Project->GetNetwork();
			const std::size_t inputSize = network.GetInputSize();
			const std::size_t outputSize = network.GetOutputSize();

			std::ostringstream resultOss;

			resultOss << std::fixed;

			for (std::size_t i = 0; i < trainData->size(); ++i) {
				if (i > 0) {
					resultOss << "\n\n";
				}

				const Matrix output = network.Forward((*trainData)[i].first);
				const float mse = network.GetOptimizer().GetLossFunction()->Forward(output, (*trainData)[i].second);

				resultOss << "입력 #" << i << ": [";
				for (std::size_t j = 0; j < inputSize; ++j) {
					resultOss << " " << (*trainData)[i].first(j, 0);
				}
				resultOss << " ]\n";

				resultOss << "정답 #" << i << ": [";
				for (std::size_t j = 0; j < outputSize; ++j) {
					resultOss << " " << (*trainData)[i].second(j, 0);
				}
				resultOss << " ]\n";

				resultOss << "출력 #" << i << ": [";
				for (std::size_t j = 0; j < outputSize; ++j) {
					resultOss << " " << output(j, 0);
				}
				resultOss << " ] (MSE " << mse << ')';
			}

			MessageDialogRef messageDialog(*m_Window, SAM_APPNAME, "실행 결과", resultOss.str(),
				MessageDialog::Information, MessageDialog::Ok);

			messageDialog->Show();
		})));

	network->AddSubItem(MenuItemSeparatorRef());
	network->AddSubItem(MenuItemRef("빠른 학습", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			const auto trainData = AskTrainData("빠른 학습");

			if (!trainData) return;

			// TODO
		})));
	network->AddSubItem(MenuItemRef("학습 및 시각화", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			const auto trainData = AskTrainData("학습 및 시각화");

			if (!trainData) return;

			// TODO
		})));
	network->AddSubItem(MenuItemRef("옵티마이저 설정", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			// TODO
		})));

	menu->AddItem(std::move(network));

	DropDownMenuItemRef help("도움말");

	help->AddSubItem(MenuItemRef("도움말 보기", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			// TODO
		})));
	help->AddSubItem(MenuItemRef("피드백 보내기", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			// TODO
		})));

	help->AddSubItem(MenuItemSeparatorRef());
	help->AddSubItem(MenuItemRef("업데이트 확인", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			// TODO
		})));
	help->AddSubItem(MenuItemRef("ShitAIMaker 정보", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			// TODO
		})));

	menu->AddItem(std::move(help));

	return menu;
}
void MainWindowHandler::UpdateText() {
	const std::string newText = std::string(m_Project->GetName()) + " - " SAM_APPTITLE;

	m_Window->SetText(m_IsSaved ? newText : '*' + newText);
}

DialogResult MainWindowHandler::AskDiscardChanges() {
	if (m_IsSaved) return DialogResult::No;

	MessageDialogRef dialog(*m_Window, SAM_APPNAME, "저장되지 않은 변경 사항이 있습니다",
		"저장되지 않은 변경 사항은 모두 삭제됩니다. 변경 사항을 저장할까요?",
		MessageDialog::Warning, MessageDialog::Yes | MessageDialog::No | MessageDialog::Cancel);

	return dialog->Show();
}
void MainWindowHandler::CreateNewProject() {
	m_Project = std::make_unique<Project>();
	m_Project->SetName("제목 없음");

	m_IsSaved = true;

	UpdateText();

	if (m_NetworkViewer) {
		dynamic_cast<NetworkViewerHandler&>(m_NetworkViewer->GetEventHandler()).
			SetTargetNetwork(m_Project->GetNetwork());
	}
}
bool MainWindowHandler::SaveProject(bool saveAs) {
	if (saveAs || m_Project->GetPath().empty()) {
		SaveFileDialogRef saveFileDialog(*m_Window, saveAs ? "다른 이름으로 저장" : "저장");

		saveFileDialog->AddFilter("프로젝트 파일(*.samp)", "*.samp");
		saveFileDialog->AddFilter("모든 파일(*.*)", "*.*");

		if (saveFileDialog->Show() != DialogResult::Ok) return false;

		m_Project->SetPath(saveFileDialog->GetPath());
	}

	try {
		m_Project->Save();

		m_IsSaved = true;

		UpdateText();

		return true;
	} catch (const std::exception& exception) {
		MessageDialogRef dialog(*m_Window, SAM_APPNAME, "프로젝트를 저장하지 못했습니다",
			std::string("저장하려는 경로가 올바른지 확인해 보세요. (") + exception.what() + ")",
			MessageDialog::Error, MessageDialog::Ok);

		dialog->Show();

		return false;
	}
}

std::optional<TrainData> MainWindowHandler::AskTrainData(std::string dialogTitle) {
	if (m_Project->GetNetwork().GetLayerCount() == 0) {
	emptyError:
		MessageDialogRef messageDialog(*m_Window, SAM_APPNAME, "올바르지 않은 네트워크 구성입니다",
			"전결합층이 적어도 1개 이상 포함되어 있는지 확인해 보세요.",
			MessageDialog::Error, MessageDialog::Ok);

		messageDialog->Show();

		return std::nullopt;
	}

	const std::size_t inputSize = m_Project->GetNetwork().GetInputSize();
	const std::size_t outputSize = m_Project->GetNetwork().GetOutputSize();

	if (inputSize == 0 || outputSize == 0) goto emptyError;

	WindowDialogRef trainDataInputDialog(*m_Window, std::move(dialogTitle),
		std::make_unique<TrainDataInputDialogHandler>(inputSize, outputSize));

	if (trainDataInputDialog->Show() == DialogResult::Ok)
		return dynamic_cast<TrainDataInputDialogHandler&>(trainDataInputDialog->GetEventHandler()).GetTrainData();
	else return std::nullopt;
}

TrainDataInputDialogHandler::TrainDataInputDialogHandler(std::size_t inputSize, std::size_t outputSize) noexcept
	: m_InputSize(inputSize), m_OutputSize(outputSize) {
	assert(inputSize > 0);
	assert(outputSize > 0);
}

bool TrainDataInputDialogHandler::HasTrainData() const noexcept {
	return m_TrainData.has_value();
}
const TrainData& TrainDataInputDialogHandler::GetTrainData() const noexcept {
	return *m_TrainData;
}

void TrainDataInputDialogHandler::OnCreate(WindowDialog& dialog) {
	m_WindowDialog = &dialog;

	m_TrainDataTextBox = &dynamic_cast<TextBox&>(dialog.AddChild(
		TextBoxRef(std::make_unique<TextBoxEventHandler>(), true)));

	m_TrainDataTextBox->SetLocation(10, 10);
	m_TrainDataTextBox->Show();

	class OkButtonHandler final : public ButtonEventHandler {
	private:
		WindowDialog& m_WindowDialog;

	public:
		OkButtonHandler(WindowDialog& windowDialog) noexcept
			: m_WindowDialog(windowDialog) {}
		OkButtonHandler(const OkButtonHandler&) = delete;
		virtual ~OkButtonHandler() override = default;

	public:
		OkButtonHandler& operator=(const OkButtonHandler&) = delete;

	public:
		virtual void OnClick(Control&) override {
			dynamic_cast<TrainDataInputDialogHandler&>(m_WindowDialog.GetEventHandler()).OnOkButtonClick();
		}
	};

	m_OkButton = &dynamic_cast<Button&>(dialog.AddChild(
		ButtonRef(std::make_unique<OkButtonHandler>(*m_WindowDialog))));

	m_OkButton->SetText("확인");
	m_OkButton->Show();

	class CancelButtonHandler final : public ButtonEventHandler {
	private:
		WindowDialog& m_WindowDialog;

	public:
		CancelButtonHandler(WindowDialog& windowDialog) noexcept
			: m_WindowDialog(windowDialog) {}
		CancelButtonHandler(const CancelButtonHandler&) = delete;
		virtual ~CancelButtonHandler() override = default;

	public:
		CancelButtonHandler& operator=(const CancelButtonHandler&) = delete;

	public:
		virtual void OnClick(Control&) override {
			dynamic_cast<TrainDataInputDialogHandler&>(m_WindowDialog.GetEventHandler()).OnCancelButtonClick();
		}
	};

	m_CancelButton = &dynamic_cast<Button&>(dialog.AddChild(
		ButtonRef(std::make_unique<CancelButtonHandler>(*m_WindowDialog))));

	m_CancelButton->SetText("취소");
	m_CancelButton->Show();

	m_WindowDialog->SetMinimumSize(400, 200);
}

void TrainDataInputDialogHandler::OnResize(WindowDialog&) {
	const auto& [clientWidth, clientHeight] = m_WindowDialog->GetClientSize();

	if (m_TrainDataTextBox) {
		m_TrainDataTextBox->SetSize(clientWidth - 20, clientHeight - (30 + 24));

		m_OkButton->SetLocation(clientWidth - (20 + 82 * 2), clientHeight - (10 + 24));
		m_OkButton->SetSize(82, 24);

		m_CancelButton->SetLocation(clientWidth - (10 + 82), clientHeight - (10 + 24));
		m_CancelButton->SetSize(82, 24);
	}
}

void TrainDataInputDialogHandler::OnOkButtonClick() {
	std::istringstream iss(m_TrainDataTextBox->GetText() + ' ');

	TrainData trainData;

	while (true) {
		TrainSample trainSample;
		Matrix& input = trainSample.first;
		Matrix& output = trainSample.second;

		input = Matrix(m_InputSize, 1);
		output = Matrix(m_OutputSize, 1);

		for (std::size_t i = 0; i < m_InputSize; ++i) {
			iss >> input(i, 0);

			if (iss.eof()) {
				if (i > 0) {
					MessageDialogRef messageDialog(m_WindowDialog->GetWindow(), SAM_APPNAME, "올바르지 않은 형식입니다",
						"입력 및 출력의 크기를 확인해 보세요.",
						MessageDialog::Error, MessageDialog::Ok);

					messageDialog->Show();

					return;
				} goto close;
			} else if (iss.fail() || iss.bad()) {
				MessageDialogRef messageDialog(m_WindowDialog->GetWindow(), SAM_APPNAME, "올바르지 않은 형식입니다",
					"숫자만 입력했는지 확인해 보세요.",
					MessageDialog::Error, MessageDialog::Ok);

				messageDialog->Show();

				return;
			}
		}

		for (std::size_t i = 0; i < m_OutputSize; ++i) {
			iss >> output(i, 0);

			if (iss.eof()) {
				MessageDialogRef messageDialog(m_WindowDialog->GetWindow(), SAM_APPNAME, "올바르지 않은 형식입니다",
					"입력 및 출력의 크기를 확인해 보세요.",
					MessageDialog::Error, MessageDialog::Ok);

				messageDialog->Show();

				return;
			} else if (iss.fail() || iss.bad()) {
				MessageDialogRef messageDialog(m_WindowDialog->GetWindow(), SAM_APPNAME, "올바르지 않은 형식입니다",
					"숫자만 입력했는지 확인해 보세요.",
					MessageDialog::Error, MessageDialog::Ok);

				messageDialog->Show();

				return;
			}
		}

		trainData.push_back(std::move(trainSample));
	}

close:
	if (trainData.empty()) {
		MessageDialogRef messageDialog(m_WindowDialog->GetWindow(), SAM_APPNAME, "올바르지 않은 형식입니다",
			"데이터를 입력했는지 확인해 보세요.",
			MessageDialog::Error, MessageDialog::Ok);

		messageDialog->Show();

		return;
	}

	m_TrainData.emplace(std::move(trainData));

	m_WindowDialog->Close(DialogResult::Ok);
}
void TrainDataInputDialogHandler::OnCancelButtonClick() {
	m_WindowDialog->Close(DialogResult::Cancel);
}