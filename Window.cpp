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

	DropDownMenuItemRef project("������Ʈ");

	project->AddSubItem(MenuItemRef("���� �����", std::make_unique<FunctionalMenuItemEventHandler>(
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
	project->AddSubItem(MenuItemRef("����", std::make_unique<FunctionalMenuItemEventHandler>(
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

			OpenFileDialogRef openFileDialog(*m_Window, "����");

			openFileDialog->AddFilter("������Ʈ ����(*.samp)", "*.samp");
			openFileDialog->AddFilter("��� ����(*.*)", "*.*");

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
				MessageDialogRef dialog(*m_Window, SAM_APPNAME, "������Ʈ�� ���� ���߽��ϴ�",
					std::string("�ùٸ� ShitAIMaker ������Ʈ �������� Ȯ���� ������. (") + exception.what() + ")",
					MessageDialog::Error, MessageDialog::Ok);

				dialog->Show();
			}
		})));
	project->AddSubItem(MenuItemRef("����", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			if (!m_IsSaved || m_Project->GetPath().empty()) {
				SaveProject();
			}
		})));
	project->AddSubItem(MenuItemRef("�ٸ� �̸����� ����", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			SaveProject(true);
		})));

	project->AddSubItem(MenuItemSeparatorRef());
	project->AddSubItem(MenuItemRef("������", std::make_unique<FunctionalMenuItemEventHandler>(
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

	DropDownMenuItemRef network("��Ʈ��ũ");

	network->AddSubItem(MenuItemRef("����", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			const auto trainData = AskTrainData("����");

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

				resultOss << "�Է� #" << i << ": [";
				for (std::size_t j = 0; j < inputSize; ++j) {
					resultOss << " " << (*trainData)[i].first(j, 0);
				}
				resultOss << " ]\n";

				resultOss << "���� #" << i << ": [";
				for (std::size_t j = 0; j < outputSize; ++j) {
					resultOss << " " << (*trainData)[i].second(j, 0);
				}
				resultOss << " ]\n";

				resultOss << "��� #" << i << ": [";
				for (std::size_t j = 0; j < outputSize; ++j) {
					resultOss << " " << output(j, 0);
				}
				resultOss << " ] (MSE " << mse << ')';
			}

			MessageDialogRef messageDialog(*m_Window, SAM_APPNAME, "���� ���", resultOss.str(),
				MessageDialog::Information, MessageDialog::Ok);

			messageDialog->Show();
		})));

	network->AddSubItem(MenuItemSeparatorRef());
	network->AddSubItem(MenuItemRef("���� �н�", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			const auto trainData = AskTrainData("���� �н�");

			if (!trainData) return;

			// TODO
		})));
	network->AddSubItem(MenuItemRef("�н� �� �ð�ȭ", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			const auto trainData = AskTrainData("�н� �� �ð�ȭ");

			if (!trainData) return;

			// TODO
		})));
	network->AddSubItem(MenuItemRef("��Ƽ������ ����", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			// TODO
		})));

	menu->AddItem(std::move(network));

	DropDownMenuItemRef help("����");

	help->AddSubItem(MenuItemRef("���� ����", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			// TODO
		})));
	help->AddSubItem(MenuItemRef("�ǵ�� ������", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			// TODO
		})));

	help->AddSubItem(MenuItemSeparatorRef());
	help->AddSubItem(MenuItemRef("������Ʈ Ȯ��", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			// TODO
		})));
	help->AddSubItem(MenuItemRef("ShitAIMaker ����", std::make_unique<FunctionalMenuItemEventHandler>(
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

	MessageDialogRef dialog(*m_Window, SAM_APPNAME, "������� ���� ���� ������ �ֽ��ϴ�",
		"������� ���� ���� ������ ��� �����˴ϴ�. ���� ������ �����ұ��?",
		MessageDialog::Warning, MessageDialog::Yes | MessageDialog::No | MessageDialog::Cancel);

	return dialog->Show();
}
void MainWindowHandler::CreateNewProject() {
	m_Project = std::make_unique<Project>();
	m_Project->SetName("���� ����");

	m_IsSaved = true;

	UpdateText();

	if (m_NetworkViewer) {
		dynamic_cast<NetworkViewerHandler&>(m_NetworkViewer->GetEventHandler()).
			SetTargetNetwork(m_Project->GetNetwork());
	}
}
bool MainWindowHandler::SaveProject(bool saveAs) {
	if (saveAs || m_Project->GetPath().empty()) {
		SaveFileDialogRef saveFileDialog(*m_Window, saveAs ? "�ٸ� �̸����� ����" : "����");

		saveFileDialog->AddFilter("������Ʈ ����(*.samp)", "*.samp");
		saveFileDialog->AddFilter("��� ����(*.*)", "*.*");

		if (saveFileDialog->Show() != DialogResult::Ok) return false;

		m_Project->SetPath(saveFileDialog->GetPath());
	}

	try {
		m_Project->Save();

		m_IsSaved = true;

		UpdateText();

		return true;
	} catch (const std::exception& exception) {
		MessageDialogRef dialog(*m_Window, SAM_APPNAME, "������Ʈ�� �������� ���߽��ϴ�",
			std::string("�����Ϸ��� ��ΰ� �ùٸ��� Ȯ���� ������. (") + exception.what() + ")",
			MessageDialog::Error, MessageDialog::Ok);

		dialog->Show();

		return false;
	}
}

std::optional<TrainData> MainWindowHandler::AskTrainData(std::string dialogTitle) {
	if (m_Project->GetNetwork().GetLayerCount() == 0) {
	emptyError:
		MessageDialogRef messageDialog(*m_Window, SAM_APPNAME, "�ùٸ��� ���� ��Ʈ��ũ �����Դϴ�",
			"���������� ��� 1�� �̻� ���ԵǾ� �ִ��� Ȯ���� ������.",
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

	m_OkButton->SetText("Ȯ��");
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

	m_CancelButton->SetText("���");
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
					MessageDialogRef messageDialog(m_WindowDialog->GetWindow(), SAM_APPNAME, "�ùٸ��� ���� �����Դϴ�",
						"�Է� �� ����� ũ�⸦ Ȯ���� ������.",
						MessageDialog::Error, MessageDialog::Ok);

					messageDialog->Show();

					return;
				} goto close;
			} else if (iss.fail() || iss.bad()) {
				MessageDialogRef messageDialog(m_WindowDialog->GetWindow(), SAM_APPNAME, "�ùٸ��� ���� �����Դϴ�",
					"���ڸ� �Է��ߴ��� Ȯ���� ������.",
					MessageDialog::Error, MessageDialog::Ok);

				messageDialog->Show();

				return;
			}
		}

		for (std::size_t i = 0; i < m_OutputSize; ++i) {
			iss >> output(i, 0);

			if (iss.eof()) {
				MessageDialogRef messageDialog(m_WindowDialog->GetWindow(), SAM_APPNAME, "�ùٸ��� ���� �����Դϴ�",
					"�Է� �� ����� ũ�⸦ Ȯ���� ������.",
					MessageDialog::Error, MessageDialog::Ok);

				messageDialog->Show();

				return;
			} else if (iss.fail() || iss.bad()) {
				MessageDialogRef messageDialog(m_WindowDialog->GetWindow(), SAM_APPNAME, "�ùٸ��� ���� �����Դϴ�",
					"���ڸ� �Է��ߴ��� Ȯ���� ������.",
					MessageDialog::Error, MessageDialog::Ok);

				messageDialog->Show();

				return;
			}
		}

		trainData.push_back(std::move(trainSample));
	}

close:
	if (trainData.empty()) {
		MessageDialogRef messageDialog(m_WindowDialog->GetWindow(), SAM_APPNAME, "�ùٸ��� ���� �����Դϴ�",
			"�����͸� �Է��ߴ��� Ȯ���� ������.",
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