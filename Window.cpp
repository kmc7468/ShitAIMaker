#include "Window.hpp"

#include "Application.hpp"
#include "NetworkViewer.hpp"
#include "Optimizer.hpp"

#include <cassert>
#include <chrono>
#include <exception>
#include <iomanip>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

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

	network->AddSubItem(MenuItemRef("�׽�Ʈ", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			const auto trainData = AskTrainData("�׽�Ʈ ������ �Է� - ����");

			if (!trainData) return;

			Network& network = m_Project->GetNetwork();
			const auto lossFunction = network.GetOptimizer().GetLossFunction();
			const std::size_t inputSize = network.GetInputSize();
			const std::size_t outputSize = network.GetOutputSize();

			std::ostringstream resultOss;

			resultOss << std::fixed;

			for (std::size_t i = 0; i < trainData->size(); ++i) {
				if (i > 0) {
					resultOss << "\n\n";
				}

				const Matrix output = network.Forward((*trainData)[i].first);
				const float mse = lossFunction->Forward(output, (*trainData)[i].second);

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
			const auto trainData = AskTrainData("�н� ������ �Է� - ���� �н�");

			if (!trainData) return;

			const auto learningRate = AskLearningRate("�н��� �Է� - ���� �н�");

			if (!learningRate) return;

			const auto epoch = AskEpoch("����ũ �Է� - ���� �н�");

			if (!epoch) return;

			Network& network = m_Project->GetNetwork();
			const auto lossFunction = network.GetOptimizer().GetLossFunction();
			const std::size_t inputSize = network.GetInputSize();
			const std::size_t outputSize = network.GetOutputSize();

			std::vector<std::pair<Matrix, float>> befores;

			for (const auto& [input, answer] : *trainData) {
				Matrix output = network.Forward(input);
				const float mse = lossFunction->Forward(output, answer);

				befores.push_back(std::make_pair(std::move(output), mse));
			}

			network.Optimize(*trainData, static_cast<std::size_t>(*epoch));

			m_NetworkViewer->Invalidate();

			std::ostringstream resultOss;

			resultOss << std::fixed;

			for (std::size_t i = 0; i < trainData->size(); ++i) {
				if (i > 0) {
					resultOss << "\n\n";
				}

				const Matrix output = network.Forward((*trainData)[i].first);
				const float mse = lossFunction->Forward(output, (*trainData)[i].second);

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

				resultOss << "�н� �� ��� #" << i << ": [";
				for (std::size_t j = 0; j < outputSize; ++j) {
					resultOss << " " << befores[i].first(j, 0);
				}
				resultOss << " ] (MSE " << befores[i].second << ")\n";

				resultOss << "�н� �� ��� #" << i << ": [";
				for (std::size_t j = 0; j < outputSize; ++j) {
					resultOss << " " << output(j, 0);
				}
				resultOss << " ] (MSE " << mse << ')';
			}

			MessageDialogRef messageDialog(*m_Window, SAM_APPNAME, "�н� ���", resultOss.str(),
				MessageDialog::Information, MessageDialog::Ok);

			messageDialog->Show();
		})));
	network->AddSubItem(MenuItemRef("�н� �� �ð�ȭ", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			const auto trainData = AskTrainData("�н� ������ �Է� - �н� �� �ð�ȭ");

			if (!trainData) return;

			const auto learningRate = AskLearningRate("�н��� �Է� - �н� �� �ð�ȭ");

			if (!learningRate) return;

			const auto epoch = AskEpoch("����ũ �Է� - �н� �� �ð�ȭ");

			if (!epoch) return;

			Network& network = m_Project->GetNetwork();
			const auto lossFunction = network.GetOptimizer().GetLossFunction();
			const std::size_t inputSize = network.GetInputSize();
			const std::size_t outputSize = network.GetOutputSize();

			std::vector<std::pair<Matrix, float>> befores;

			for (const auto& [input, answer] : *trainData) {
				Matrix output = network.Forward(input);
				const float mse = lossFunction->Forward(output, answer);

				befores.push_back(std::make_pair(std::move(output), mse));
			}

			const std::size_t defaultEpoch = *epoch / 10;
			const std::size_t lastEpoch = defaultEpoch + *epoch % 10;

			for (std::size_t i = 0; i < 10; ++i) {
				using namespace std::chrono_literals;

				network.Optimize(*trainData, i < 9 ? defaultEpoch : lastEpoch);

				m_NetworkViewer->Invalidate();

				std::this_thread::sleep_for(100ms);
			}

			std::ostringstream resultOss;

			resultOss << std::fixed;

			for (std::size_t i = 0; i < trainData->size(); ++i) {
				if (i > 0) {
					resultOss << "\n\n";
				}

				const Matrix output = network.Forward((*trainData)[i].first);
				const float mse = lossFunction->Forward(output, (*trainData)[i].second);

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

				resultOss << "�н� �� ��� #" << i << ": [";
				for (std::size_t j = 0; j < outputSize; ++j) {
					resultOss << " " << befores[i].first(j, 0);
				}
				resultOss << " ] (MSE " << befores[i].second << ")\n";

				resultOss << "�н� �� ��� #" << i << ": [";
				for (std::size_t j = 0; j < outputSize; ++j) {
					resultOss << " " << output(j, 0);
				}
				resultOss << " ] (MSE " << mse << ')';
			}

			MessageDialogRef messageDialog(*m_Window, SAM_APPNAME, "�н� ���", resultOss.str(),
				MessageDialog::Information, MessageDialog::Ok);

			messageDialog->Show();
		})));
	network->AddSubItem(MenuItemRef("��Ƽ������ ����", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			// TODO
		})));

	network->AddSubItem(MenuItemSeparatorRef());
	network->AddSubItem(MenuItemRef("���̾� �߰�", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			// TODO
		})));
	network->AddSubItem(MenuItemRef("�Ķ���� �ʱ�ȭ", std::make_unique<FunctionalMenuItemEventHandler>(
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
std::optional<float> MainWindowHandler::AskLearningRate(std::string dialogTitle) {
	WindowDialogRef learningRateInputDialog(*m_Window, std::move(dialogTitle),
		std::make_unique<LearningRateInputDialogHandler>());

	if (learningRateInputDialog->Show() == DialogResult::Ok)
		return dynamic_cast<LearningRateInputDialogHandler&>(
			learningRateInputDialog->GetEventHandler()).GetLearningRate();
	else return std::nullopt;
}
std::optional<std::size_t> MainWindowHandler::AskEpoch(std::string dialogTitle) {
	WindowDialogRef epochInputDialog(*m_Window, std::move(dialogTitle), std::make_unique<EpochInputDialogHandler>());

	if (epochInputDialog->Show() == DialogResult::Ok)
		return dynamic_cast<EpochInputDialogHandler&>(epochInputDialog->GetEventHandler()).GetEpoch();
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

bool LearningRateInputDialogHandler::HasLearningRate() const noexcept {
	return m_LearningRate.has_value();
}
float LearningRateInputDialogHandler::GetLearningRate() const noexcept {
	return *m_LearningRate;
}

void LearningRateInputDialogHandler::OnCreate(WindowDialog& dialog) {
	m_WindowDialog = &dialog;

	class LearningRateTextBoxHandler final : public TextBoxEventHandler {
	private:
		WindowDialog& m_WindowDialog;

	public:
		LearningRateTextBoxHandler(WindowDialog& windowDialog) noexcept
			: m_WindowDialog(windowDialog) {}
		LearningRateTextBoxHandler(const LearningRateTextBoxHandler&) = delete;
		virtual ~LearningRateTextBoxHandler() override = default;

	public:
		LearningRateTextBoxHandler& operator=(const LearningRateTextBoxHandler&) = delete;

	public:
		virtual void OnKeyUp(Control&, Key key) override {
			if (key == Key::Enter) {
				dynamic_cast<LearningRateInputDialogHandler&>(m_WindowDialog.GetEventHandler()).OnOkButtonClick();
			}
		}
	};

	m_LearningRateTextBox = &dynamic_cast<TextBox&>(dialog.AddChild(
		TextBoxRef(std::make_unique<LearningRateTextBoxHandler>(*m_WindowDialog))));

	m_LearningRateTextBox->SetLocation(10, 10);
	m_LearningRateTextBox->Show();

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
			dynamic_cast<LearningRateInputDialogHandler&>(m_WindowDialog.GetEventHandler()).OnOkButtonClick();
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
			dynamic_cast<LearningRateInputDialogHandler&>(m_WindowDialog.GetEventHandler()).OnCancelButtonClick();
		}
	};

	m_CancelButton = &dynamic_cast<Button&>(dialog.AddChild(
		ButtonRef(std::make_unique<CancelButtonHandler>(*m_WindowDialog))));

	m_CancelButton->SetText("���");
	m_CancelButton->Show();

	m_WindowDialog->SetMinimumSize(400, 130);
}

void LearningRateInputDialogHandler::OnResize(WindowDialog& dialog) {
	const auto& [clientWidth, clientHeight] = m_WindowDialog->GetClientSize();

	if (m_LearningRateTextBox) {
		m_LearningRateTextBox->SetSize(clientWidth - 20, 24);

		m_OkButton->SetLocation(clientWidth - (20 + 82 * 2), clientHeight - (10 + 24));
		m_OkButton->SetSize(82, 24);

		m_CancelButton->SetLocation(clientWidth - (10 + 82), clientHeight - (10 + 24));
		m_CancelButton->SetSize(82, 24);
	}
}

void LearningRateInputDialogHandler::OnOkButtonClick() {
	std::istringstream iss(m_LearningRateTextBox->GetText() + ' ');

	float learningRate;
	iss >> learningRate;

	if (iss.eof()) {
		MessageDialogRef messageDialog(m_WindowDialog->GetWindow(), SAM_APPNAME, "�ùٸ��� ���� �����Դϴ�",
			"�н����� �Է��ߴ��� Ȯ���� ������.",
			MessageDialog::Error, MessageDialog::Ok);

		messageDialog->Show();
	} else if (iss.fail() || iss.bad() || learningRate <= 0 || learningRate > 1) {
		MessageDialogRef messageDialog(m_WindowDialog->GetWindow(), SAM_APPNAME, "�ùٸ��� ���� �����Դϴ�",
			"�н����� 0 �ʰ� 1 �̸��� �Ǽ����� Ȯ���� ������.",
			MessageDialog::Error, MessageDialog::Ok);

		messageDialog->Show();
	} else {
		m_LearningRate.emplace(learningRate);

		m_WindowDialog->Close(DialogResult::Ok);
	}
}
void LearningRateInputDialogHandler::OnCancelButtonClick() {
	m_WindowDialog->Close(DialogResult::Cancel);
}

bool EpochInputDialogHandler::HasEpoch() const noexcept {
	return m_Epoch.has_value();
}
std::size_t EpochInputDialogHandler::GetEpoch() const noexcept {
	return *m_Epoch;
}

void EpochInputDialogHandler::OnCreate(WindowDialog& dialog) {
	m_WindowDialog = &dialog;

	class EpochTextBoxHandler final : public TextBoxEventHandler {
	private:
		WindowDialog& m_WindowDialog;

	public:
		EpochTextBoxHandler(WindowDialog& windowDialog) noexcept
			: m_WindowDialog(windowDialog) {}
		EpochTextBoxHandler(const EpochTextBoxHandler&) = delete;
		virtual ~EpochTextBoxHandler() override = default;

	public:
		EpochTextBoxHandler& operator=(const EpochTextBoxHandler&) = delete;

	public:
		virtual void OnKeyUp(Control&, Key key) override {
			if (key == Key::Enter) {
				dynamic_cast<EpochInputDialogHandler&>(m_WindowDialog.GetEventHandler()).OnOkButtonClick();
			}
		}
	};

	m_EpochTextBox = &dynamic_cast<TextBox&>(dialog.AddChild(
		TextBoxRef(std::make_unique<EpochTextBoxHandler>(*m_WindowDialog))));

	m_EpochTextBox->SetLocation(10, 10);
	m_EpochTextBox->Show();

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
			dynamic_cast<EpochInputDialogHandler&>(m_WindowDialog.GetEventHandler()).OnOkButtonClick();
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
			dynamic_cast<EpochInputDialogHandler&>(m_WindowDialog.GetEventHandler()).OnCancelButtonClick();
		}
	};

	m_CancelButton = &dynamic_cast<Button&>(dialog.AddChild(
		ButtonRef(std::make_unique<CancelButtonHandler>(*m_WindowDialog))));

	m_CancelButton->SetText("���");
	m_CancelButton->Show();

	m_WindowDialog->SetMinimumSize(400, 130);
}

void EpochInputDialogHandler::OnResize(WindowDialog& dialog) {
	const auto& [clientWidth, clientHeight] = m_WindowDialog->GetClientSize();

	if (m_EpochTextBox) {
		m_EpochTextBox->SetSize(clientWidth - 20, 24);

		m_OkButton->SetLocation(clientWidth - (20 + 82 * 2), clientHeight - (10 + 24));
		m_OkButton->SetSize(82, 24);

		m_CancelButton->SetLocation(clientWidth - (10 + 82), clientHeight - (10 + 24));
		m_CancelButton->SetSize(82, 24);
	}
}

void EpochInputDialogHandler::OnOkButtonClick() {
	std::istringstream iss(m_EpochTextBox->GetText() + ' ');

	std::size_t epoch;
	iss >> epoch;

	if (iss.eof()) {
		MessageDialogRef messageDialog(m_WindowDialog->GetWindow(), SAM_APPNAME, "�ùٸ��� ���� �����Դϴ�",
			"����ũ�� �Է��ߴ��� Ȯ���� ������.",
			MessageDialog::Error, MessageDialog::Ok);

		messageDialog->Show();
	} else if (iss.fail() || iss.bad() || epoch == 0) {
		MessageDialogRef messageDialog(m_WindowDialog->GetWindow(), SAM_APPNAME, "�ùٸ��� ���� �����Դϴ�",
			"����ũ�� �ڿ������� Ȯ���� ������.",
			MessageDialog::Error, MessageDialog::Ok);

		messageDialog->Show();
	} else {
		m_Epoch.emplace(epoch);

		m_WindowDialog->Close(DialogResult::Ok);
	}
}
void EpochInputDialogHandler::OnCancelButtonClick() {
	m_WindowDialog->Close(DialogResult::Cancel);
}