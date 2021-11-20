#include "Window.hpp"

#include "Application.hpp"
#include "Layer.hpp"
#include "Matrix.hpp"
#include "NetworkViewer.hpp"
#include "Optimizer.hpp"

#include <cassert>
#include <chrono>
#include <exception>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#define SAM_DONETEST 0
#define SAM_DONEFASTOPTIMIZING 1
#define SAM_DONEOPTIMIZING 2

namespace {
	void PrintInputOrOutput(std::ostream& stream, const char* title, std::size_t index, const Matrix& matrix) {
		stream << title << " #" << index << ": [";
		for (std::size_t i = 0; i < matrix.GetRowSize(); ++i) {
			stream << " " << matrix(i, 0);
		}
		stream << " ]";
	}
}

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

class TrainDataInputDialogHandler final : public WindowDialogEventHandler {
private:
	WindowDialog* m_WindowDialog = nullptr;

	TextBox* m_TrainDataTextBox = nullptr;
	Button* m_OkButton = nullptr;
	Button* m_CancelButton = nullptr;

	std::size_t m_InputSize, m_OutputSize;
	std::optional<TrainData> m_TrainData;

public:
	TrainDataInputDialogHandler(std::size_t inputSize, std::size_t outputSize) noexcept
		: m_InputSize(inputSize), m_OutputSize(outputSize) {
		assert(inputSize > 0);
		assert(outputSize > 0);
	}
	TrainDataInputDialogHandler(const TrainDataInputDialogHandler&) = delete;
	virtual ~TrainDataInputDialogHandler() override = default;

public:
	TrainDataInputDialogHandler& operator=(const TrainDataInputDialogHandler&) = delete;

public:
	bool HasTrainData() const noexcept {
		return m_TrainData.has_value();
	}
	const TrainData& GetTrainData() const noexcept {
		return *m_TrainData;
	}

public:
	virtual void OnCreate(WindowDialog& dialog) override {
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

	virtual void OnResize(WindowDialog&) override {
		const auto& [clientWidth, clientHeight] = m_WindowDialog->GetClientSize();

		if (m_TrainDataTextBox) {
			m_TrainDataTextBox->SetSize(clientWidth - 20, clientHeight - (30 + 24));

			m_OkButton->SetLocation(clientWidth - (20 + 82 * 2), clientHeight - (10 + 24));
			m_OkButton->SetSize(82, 24);

			m_CancelButton->SetLocation(clientWidth - (10 + 82), clientHeight - (10 + 24));
			m_CancelButton->SetSize(82, 24);
		}
	}

public:
	void OnOkButtonClick() {
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
	void OnCancelButtonClick() {
		m_WindowDialog->Close(DialogResult::Cancel);
	}
};

class LearningRateInputDialogHandler final : public WindowDialogEventHandler {
private:
	WindowDialog* m_WindowDialog = nullptr;

	TextBox* m_LearningRateTextBox = nullptr;
	Button* m_OkButton = nullptr;
	Button* m_CancelButton = nullptr;

	std::optional<float> m_LearningRate;

public:
	LearningRateInputDialogHandler() noexcept = default;
	LearningRateInputDialogHandler(const LearningRateInputDialogHandler&) = delete;
	virtual ~LearningRateInputDialogHandler() override = default;

public:
	LearningRateInputDialogHandler& operator=(const LearningRateInputDialogHandler&) = delete;

public:
	bool HasLearningRate() const noexcept {
		return m_LearningRate.has_value();
	}
	float GetLearningRate() const noexcept {
		return *m_LearningRate;
	}

public:
	virtual void OnCreate(WindowDialog& dialog) override {
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

	virtual void OnResize(WindowDialog&) override {
		const auto& [clientWidth, clientHeight] = m_WindowDialog->GetClientSize();

		if (m_LearningRateTextBox) {
			m_LearningRateTextBox->SetSize(clientWidth - 20, 24);

			m_OkButton->SetLocation(clientWidth - (20 + 82 * 2), clientHeight - (10 + 24));
			m_OkButton->SetSize(82, 24);

			m_CancelButton->SetLocation(clientWidth - (10 + 82), clientHeight - (10 + 24));
			m_CancelButton->SetSize(82, 24);
		}
	}

public:
	void OnOkButtonClick() {
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
	void OnCancelButtonClick() {
		m_WindowDialog->Close(DialogResult::Cancel);
	}
};

class EpochInputDialogHandler final : public WindowDialogEventHandler {
private:
	WindowDialog* m_WindowDialog = nullptr;

	TextBox* m_EpochTextBox = nullptr;
	Button* m_OkButton = nullptr;
	Button* m_CancelButton = nullptr;

	std::optional<std::size_t> m_Epoch;

public:
	EpochInputDialogHandler() noexcept = default;
	EpochInputDialogHandler(const EpochInputDialogHandler&) = delete;
	virtual ~EpochInputDialogHandler() override = default;

public:
	EpochInputDialogHandler& operator=(const EpochInputDialogHandler&) = delete;

public:
	bool HasEpoch() const noexcept {
		return m_Epoch.has_value();
	}
	std::size_t GetEpoch() const noexcept {
		return *m_Epoch;
	}

public:
	virtual void OnCreate(WindowDialog& dialog) override {
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

	virtual void OnResize(WindowDialog&) override {
		const auto& [clientWidth, clientHeight] = m_WindowDialog->GetClientSize();

		if (m_EpochTextBox) {
			m_EpochTextBox->SetSize(clientWidth - 20, 24);

			m_OkButton->SetLocation(clientWidth - (20 + 82 * 2), clientHeight - (10 + 24));
			m_OkButton->SetSize(82, 24);

			m_CancelButton->SetLocation(clientWidth - (10 + 82), clientHeight - (10 + 24));
			m_CancelButton->SetSize(82, 24);
		}
	}

public:
	void OnOkButtonClick() {
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
	void OnCancelButtonClick() {
		m_WindowDialog->Close(DialogResult::Cancel);
	}
};

class OptimizerOptionDialogHandler final : public WindowDialogEventHandler {
private:
	WindowDialog* m_WindowDialog = nullptr;

	ComboBox* m_OptimizerNameComboBox = nullptr;
	ComboBox* m_LossFunctionNameComboBox = nullptr;
	Button* m_OkButton = nullptr;
	Button* m_ApplyButton = nullptr;
	Button* m_CancelButton = nullptr;

	std::size_t prevOptimizerNameComboBoxIndex = ComboBox::NoSelected;
	std::size_t prevLossFunctionNameComboBoxIndex = ComboBox::NoSelected;

	TextBox* m_LearningRateTextBox = nullptr;

	Network& m_Network;
	bool m_IsOptimzierEdited = false;

	std::unique_ptr<SGDOptimizer> m_SGDOptimizer = std::make_unique<SGDOptimizer>();

public:
	OptimizerOptionDialogHandler(Network& network)
		: m_Network(network) {
		if (!network.HasOptimizer()) return;

		const Optimizer& optimizer = m_Network.GetOptimizer();
		const std::string_view optimizerName = optimizer.GetName();

		if (optimizerName == "SGDOptimizer") {
			m_SGDOptimizer = std::unique_ptr<SGDOptimizer>(
				static_cast<SGDOptimizer*>(optimizer.Copy().release()));
		}
	}
	OptimizerOptionDialogHandler(const OptimizerOptionDialogHandler&) = delete;
	virtual ~OptimizerOptionDialogHandler() override = default;

public:
	OptimizerOptionDialogHandler& operator=(const OptimizerOptionDialogHandler&) = delete;

public:
	virtual void OnCreate(WindowDialog& dialog) override {
		m_WindowDialog = &dialog;

		class OptimizerNameComboBoxHandler final : public ComboBoxEventHandler {
		private:
			WindowDialog& m_WindowDialog;

		public:
			OptimizerNameComboBoxHandler(WindowDialog& windowDialog) noexcept
				: m_WindowDialog(windowDialog) {}
			OptimizerNameComboBoxHandler(const OptimizerNameComboBoxHandler&) = delete;
			virtual ~OptimizerNameComboBoxHandler() override = default;

		public:
			OptimizerNameComboBoxHandler& operator=(const OptimizerNameComboBoxHandler&) = delete;

		public:
			virtual void OnItemSelected(ComboBox&, std::size_t index) override {
				dynamic_cast<OptimizerOptionDialogHandler&>(
					m_WindowDialog.GetEventHandler()).OnOptimizerNameComboBoxItemChanged(index);
			}
		};

		m_OptimizerNameComboBox = &dynamic_cast<ComboBox&>(dialog.AddChild(
			ComboBoxRef(std::make_unique<OptimizerNameComboBoxHandler>(*m_WindowDialog))));

		m_OptimizerNameComboBox->SetLocation(10, 10);
		m_OptimizerNameComboBox->Show();

		m_OptimizerNameComboBox->AddItem("Ȯ���� ��� �ϰ���");

		class LossFunctionNameComboBoxHandler final : public ComboBoxEventHandler {
		private:
			WindowDialog& m_WindowDialog;

		public:
			LossFunctionNameComboBoxHandler(WindowDialog& windowDialog) noexcept
				: m_WindowDialog(windowDialog) {}
			LossFunctionNameComboBoxHandler(const LossFunctionNameComboBoxHandler&) = delete;
			virtual ~LossFunctionNameComboBoxHandler() override = default;

		public:
			LossFunctionNameComboBoxHandler& operator=(const LossFunctionNameComboBoxHandler&) = delete;

		public:
			virtual void OnItemSelected(ComboBox&, std::size_t index) override {
				dynamic_cast<OptimizerOptionDialogHandler&>(
					m_WindowDialog.GetEventHandler()).OnLossFunctionNameComboBoxItemChanged(index);
			}
		};

		m_LossFunctionNameComboBox = &dynamic_cast<ComboBox&>(dialog.AddChild(
			ComboBoxRef(std::make_unique<LossFunctionNameComboBoxHandler>(*m_WindowDialog))));

		m_LossFunctionNameComboBox->SetLocation(10, 10 + (10 + 24));
		m_LossFunctionNameComboBox->Show();
		m_LossFunctionNameComboBox->SetEnabled(false);

		m_LossFunctionNameComboBox->AddItem("MSE");

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
				dynamic_cast<OptimizerOptionDialogHandler&>(m_WindowDialog.GetEventHandler()).OnOkButtonClick();
			}
		};

		m_OkButton = &dynamic_cast<Button&>(dialog.AddChild(
			ButtonRef(std::make_unique<OkButtonHandler>(*m_WindowDialog))));

		m_OkButton->SetText("Ȯ��");
		m_OkButton->Show();

		class ApplyButtonHandler final : public ButtonEventHandler {
		private:
			WindowDialog& m_WindowDialog;

		public:
			ApplyButtonHandler(WindowDialog& windowDialog) noexcept
				: m_WindowDialog(windowDialog) {}
			ApplyButtonHandler(const ApplyButtonHandler&) = delete;
			virtual ~ApplyButtonHandler() override = default;

		public:
			ApplyButtonHandler& operator=(const ApplyButtonHandler&) = delete;

		public:
			virtual void OnClick(Control&) override {
				dynamic_cast<OptimizerOptionDialogHandler&>(m_WindowDialog.GetEventHandler()).OnApplyButtonClick();
			}
		};

		m_ApplyButton = &dynamic_cast<Button&>(dialog.AddChild(
			ButtonRef(std::make_unique<ApplyButtonHandler>(*m_WindowDialog))));

		m_ApplyButton->SetText("����");
		m_ApplyButton->Show();
		m_ApplyButton->SetEnabled(false);

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
				dynamic_cast<OptimizerOptionDialogHandler&>(m_WindowDialog.GetEventHandler()).OnCancelButtonClick();
			}
		};

		m_CancelButton = &dynamic_cast<Button&>(dialog.AddChild(
			ButtonRef(std::make_unique<CancelButtonHandler>(*m_WindowDialog))));

		m_CancelButton->SetText("���");
		m_CancelButton->Show();

		class LearingRateTextBoxHandler final : public TextBoxEventHandler {
		private:
			WindowDialog& m_WindowDialog;

		public:
			LearingRateTextBoxHandler(WindowDialog& windowDialog) noexcept
				: m_WindowDialog(windowDialog) {}
			LearingRateTextBoxHandler(const LearingRateTextBoxHandler&) = delete;
			virtual ~LearingRateTextBoxHandler() override = default;

		public:
			LearingRateTextBoxHandler& operator=(const LearingRateTextBoxHandler&) = delete;

		public:
			virtual void OnTextChanged(TextBox&) override {
				dynamic_cast<OptimizerOptionDialogHandler&>(
					m_WindowDialog.GetEventHandler()).OnLearningRateTextBoxTextChanged();
			}
		};

		m_LearningRateTextBox = &dynamic_cast<TextBox&>(dialog.AddChild(
			TextBoxRef(std::make_unique<LearingRateTextBoxHandler>(*m_WindowDialog))));

		m_LearningRateTextBox->SetLocation(10, 10 + (20 + 24 * 2));

		m_WindowDialog->SetMinimumSize(400, 160);

		if (m_Network.HasOptimizer()) {
			const Optimizer& optimizer = m_Network.GetOptimizer();
			const std::string_view optimizerName = optimizer.GetName();

			if (optimizerName == "SGDOptimizer") {
				m_OptimizerNameComboBox->SetSelectedItemIndex(0);

				OnOptimizerNameComboBoxItemChanged(0);

				m_ApplyButton->SetEnabled(false);
			}

			const std::string_view lossFunctionName = optimizer.GetLossFunction()->GetName();

			if (lossFunctionName == "MSE") {
				m_LossFunctionNameComboBox->SetSelectedItemIndex(0);

				OnLossFunctionNameComboBoxItemChanged(0);

				m_ApplyButton->SetEnabled(false);
			}

			m_LossFunctionNameComboBox->SetEnabled(true);
		}
	}

	virtual void OnResize(WindowDialog&) override {
		const auto& [clientWidth, clientHeight] = m_WindowDialog->GetClientSize();

		if (m_OptimizerNameComboBox) {
			m_OptimizerNameComboBox->SetSize(clientWidth - 20, 24);

			m_LossFunctionNameComboBox->SetSize(clientWidth - 20, 24);

			m_OkButton->SetLocation(clientWidth - (30 + 82 * 3), clientHeight - (10 + 24));
			m_OkButton->SetSize(82, 24);

			m_ApplyButton->SetLocation(clientWidth - (20 + 82 * 2), clientHeight - (10 + 24));
			m_ApplyButton->SetSize(82, 24);

			m_CancelButton->SetLocation(clientWidth - (10 + 82), clientHeight - (10 + 24));
			m_CancelButton->SetSize(82, 24);

			m_LearningRateTextBox->SetSize(clientWidth - 20, 24);
		}
	}

public:
	void OnOptimizerNameComboBoxItemChanged(std::size_t index) {
		if (prevOptimizerNameComboBoxIndex == index) return;

		if (index == 0) {
			m_LossFunctionNameComboBox->SetEnabled(true);

			m_ApplyButton->SetEnabled(true);

			m_LearningRateTextBox->SetText(std::to_string(m_SGDOptimizer->GetLearningRate()));
			m_LearningRateTextBox->Show();

			m_WindowDialog->SetMinimumSize(400, 190);
		}

		prevOptimizerNameComboBoxIndex = index;
	}
	void OnLossFunctionNameComboBoxItemChanged(std::size_t index) {
		if (prevLossFunctionNameComboBoxIndex == index) return;

		std::shared_ptr<const LossFunction> newLossFunction = nullptr;

		if (index == 0) {
			newLossFunction = MSE;
		}

		m_SGDOptimizer->SetLossFunction(newLossFunction);

		m_ApplyButton->SetEnabled(true);

		prevLossFunctionNameComboBoxIndex = index;
	}

	void OnLearningRateTextBoxTextChanged() {
		m_ApplyButton->SetEnabled(true);
	}

	void OnOkButtonClick() {
		if (m_ApplyButton->GetEnabled()) {
			OnApplyButtonClick();
		}

		OnCancelButtonClick();
	}
	void OnApplyButtonClick() {
		if (m_LossFunctionNameComboBox->GetSelectedItemIndex() == ComboBox::NoSelected) {
			MessageDialogRef messageDialog(m_WindowDialog->GetWindow(), SAM_APPNAME, "�ս� �Լ��� ���õ��� �ʾҽ��ϴ�", {},
				MessageDialog::Error, MessageDialog::Ok);

			messageDialog->Show();

			return;
		}

		switch (m_OptimizerNameComboBox->GetSelectedItemIndex()) {
		case 0: {
			const auto learningRate = GetLearningRate();

			if (!learningRate) return;

			m_SGDOptimizer->SetLearningRate(*learningRate);
			m_Network.SetOptimizer(m_SGDOptimizer->Copy());

			break;
		}
		}

		m_ApplyButton->SetEnabled(false);

		m_IsOptimzierEdited = true;
	}
	void OnCancelButtonClick() {
		m_WindowDialog->Close(m_IsOptimzierEdited ? DialogResult::Ok : DialogResult::Cancel);
	}

private:
	std::optional<float> GetLearningRate() {
		std::istringstream iss(m_LearningRateTextBox->GetText() + ' ');

		float learningRate;
		iss >> learningRate;

		if (iss.eof()) {
			MessageDialogRef messageDialog(m_WindowDialog->GetWindow(), SAM_APPNAME, "�ùٸ��� ���� �����Դϴ�",
				"�н����� �Է��ߴ��� Ȯ���� ������.",
				MessageDialog::Error, MessageDialog::Ok);

			messageDialog->Show();

			return std::nullopt;
		} else if (iss.fail() || iss.bad() || learningRate <= 0 || learningRate > 1) {
			MessageDialogRef messageDialog(m_WindowDialog->GetWindow(), SAM_APPNAME, "�ùٸ��� ���� �����Դϴ�",
				"�н����� 0 �ʰ� 1 ������ �Ǽ����� Ȯ���� ������.",
				MessageDialog::Error, MessageDialog::Ok);

			messageDialog->Show();

			return std::nullopt;
		} else return learningRate;
	}
};

class InputOrOutputSizeInputDialogHandler final : public WindowDialogEventHandler {
private:
	WindowDialog* m_WindowDialog = nullptr;

	TextBox* m_InputOrOutputSizeTextBox = nullptr;
	Button* m_OkButton = nullptr;
	Button* m_CancelButton = nullptr;

	std::optional<std::size_t> m_InputOrOutputSize;

public:
	InputOrOutputSizeInputDialogHandler() noexcept = default;
	InputOrOutputSizeInputDialogHandler(const InputOrOutputSizeInputDialogHandler&) = delete;
	virtual ~InputOrOutputSizeInputDialogHandler() override = default;

public:
	InputOrOutputSizeInputDialogHandler& operator=(const InputOrOutputSizeInputDialogHandler&) = delete;

public:
	bool HasInputOrOutputSize() const noexcept {
		return m_InputOrOutputSize.has_value();
	}
	std::size_t GetInputOrOutputSize() const noexcept {
		return *m_InputOrOutputSize;
	}

public:
	virtual void OnCreate(WindowDialog& dialog) override {
		m_WindowDialog = &dialog;

		class InputOrOutputSizeTextBoxHandler final : public TextBoxEventHandler {
		private:
			WindowDialog& m_WindowDialog;

		public:
			InputOrOutputSizeTextBoxHandler(WindowDialog& windowDialog) noexcept
				: m_WindowDialog(windowDialog) {}
			InputOrOutputSizeTextBoxHandler(const InputOrOutputSizeTextBoxHandler&) = delete;
			virtual ~InputOrOutputSizeTextBoxHandler() override = default;

		public:
			InputOrOutputSizeTextBoxHandler& operator=(const InputOrOutputSizeTextBoxHandler&) = delete;

		public:
			virtual void OnKeyUp(Control&, Key key) override {
				if (key == Key::Enter) {
					dynamic_cast<InputOrOutputSizeInputDialogHandler&>(
						m_WindowDialog.GetEventHandler()).OnOkButtonClick();
				}
			}
		};

		m_InputOrOutputSizeTextBox = &dynamic_cast<TextBox&>(dialog.AddChild(
			TextBoxRef(std::make_unique<InputOrOutputSizeTextBoxHandler>(*m_WindowDialog))));

		m_InputOrOutputSizeTextBox->SetLocation(10, 10);
		m_InputOrOutputSizeTextBox->Show();

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
				dynamic_cast<InputOrOutputSizeInputDialogHandler&>(
					m_WindowDialog.GetEventHandler()).OnOkButtonClick();
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
				dynamic_cast<InputOrOutputSizeInputDialogHandler&>(
					m_WindowDialog.GetEventHandler()).OnCancelButtonClick();
			}
		};

		m_CancelButton = &dynamic_cast<Button&>(dialog.AddChild(
			ButtonRef(std::make_unique<CancelButtonHandler>(*m_WindowDialog))));

		m_CancelButton->SetText("���");
		m_CancelButton->Show();

		m_WindowDialog->SetMinimumSize(400, 130);
	}

	virtual void OnResize(WindowDialog&) override {
		const auto& [clientWidth, clientHeight] = m_WindowDialog->GetClientSize();

		if (m_InputOrOutputSizeTextBox) {
			m_InputOrOutputSizeTextBox->SetSize(clientWidth - 20, 24);

			m_OkButton->SetLocation(clientWidth - (20 + 82 * 2), clientHeight - (10 + 24));
			m_OkButton->SetSize(82, 24);

			m_CancelButton->SetLocation(clientWidth - (10 + 82), clientHeight - (10 + 24));
			m_CancelButton->SetSize(82, 24);
		}
	}

public:
	void OnOkButtonClick() {
		std::istringstream iss(m_InputOrOutputSizeTextBox->GetText() + ' ');

		std::size_t inputOrOutputSize;
		iss >> inputOrOutputSize;

		if (iss.eof()) {
			MessageDialogRef messageDialog(m_WindowDialog->GetWindow(), SAM_APPNAME, "�ùٸ��� ���� �����Դϴ�",
				"ũ�⸦ �Է��ߴ��� Ȯ���� ������.",
				MessageDialog::Error, MessageDialog::Ok);

			messageDialog->Show();
		} else if (iss.fail() || iss.bad() || inputOrOutputSize == 0) {
			MessageDialogRef messageDialog(m_WindowDialog->GetWindow(), SAM_APPNAME, "�ùٸ��� ���� �����Դϴ�",
				"ũ�Ⱑ �ڿ������� Ȯ���� ������.",
				MessageDialog::Error, MessageDialog::Ok);

			messageDialog->Show();
		} else {
			m_InputOrOutputSize.emplace(inputOrOutputSize);

			m_WindowDialog->Close(DialogResult::Ok);
		}
	}
	void OnCancelButtonClick() {
		m_WindowDialog->Close(DialogResult::Cancel);
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

void MainWindowHandler::OnReceiveMessage(Control&, std::size_t message, std::optional<std::any> argument) {
	switch (message) {
	case SAM_DONETEST:
		DoneTestOperation(std::any_cast<std::string>(*argument));

		break;

	case SAM_DONEFASTOPTIMIZING:
		DoneFastOptimizingOperation(std::any_cast<std::string>(*argument));

		break;

	case SAM_DONEOPTIMIZING:
		if (argument) {
			DoneOptimizingOperation(std::any_cast<std::string>(*argument));
		} else {
			DoneOptimizingOperation();
		}

		break;
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

			StartOperation();

			m_Thread = std::jthread([=]() {
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

					PrintInputOrOutput(resultOss, "�Է�", i, (*trainData)[i].first);
					resultOss << '\n';

					PrintInputOrOutput(resultOss, "����", i, (*trainData)[i].second);
					resultOss << '\n';

					PrintInputOrOutput(resultOss, "���", i, output);
					resultOss << " (MSE " << mse << ')';
				}

				m_Window->SendMessage(SAM_DONETEST, resultOss.str());
			});
		})));

	network->AddSubItem(MenuItemSeparatorRef());
	network->AddSubItem(MenuItemRef("���� �н�", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			if (!m_Project->GetNetwork().HasOptimizer()) {
				MessageDialogRef messageDialog(*m_Window, SAM_APPNAME, "��Ƽ�������� �����ϴ�",
					"��Ƽ�������� �����ߴ��� Ȯ���� ������.",
					MessageDialog::Error, MessageDialog::Ok);

				messageDialog->Show();

				return;
			}

			const auto trainData = AskTrainData("�н� ������ �Է� - ���� �н�");

			if (!trainData) return;

			const auto epoch = AskEpoch("����ũ �Է� - ���� �н�");

			if (!epoch) return;

			StartOperation();

			m_Thread = std::jthread([=]() {
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

				std::ostringstream resultOss;

				resultOss << std::fixed;

				for (std::size_t i = 0; i < trainData->size(); ++i) {
					if (i > 0) {
						resultOss << "\n\n";
					}

					const Matrix output = network.Forward((*trainData)[i].first);
					const float mse = lossFunction->Forward(output, (*trainData)[i].second);

					PrintInputOrOutput(resultOss, "�Է�", i, (*trainData)[i].first);
					resultOss << '\n';

					PrintInputOrOutput(resultOss, "����", i, (*trainData)[i].second);
					resultOss << '\n';

					PrintInputOrOutput(resultOss, "�н� �� ���", i, befores[i].first);
					resultOss << " (MSE " << befores[i].second << ")\n";

					PrintInputOrOutput(resultOss, "�н� �� ���", i, output);
					resultOss << " (MSE " << mse << ')';
				}

				m_Window->SendMessage(SAM_DONEFASTOPTIMIZING, resultOss.str());
			});
		})));
	network->AddSubItem(MenuItemRef("�н� �� �ð�ȭ", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			if (!m_Project->GetNetwork().HasOptimizer()) {
				MessageDialogRef messageDialog(*m_Window, SAM_APPNAME, "��Ƽ�������� �����ϴ�",
					"��Ƽ�������� �����ߴ��� Ȯ���� ������.",
					MessageDialog::Error, MessageDialog::Ok);

				messageDialog->Show();

				return;
			}

			const auto trainData = AskTrainData("�н� ������ �Է� - �н� �� �ð�ȭ");

			if (!trainData) return;

			const auto epoch = AskEpoch("����ũ �Է� - �н� �� �ð�ȭ");

			if (!epoch) return;

			StartOperation();

			m_Thread = std::jthread([=]() {
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

				if (defaultEpoch == 0) {
					network.Optimize(*trainData, lastEpoch);
				} else {
					for (std::size_t i = 0; i < 10; ++i) {
						using namespace std::chrono_literals;

						network.Optimize(*trainData, i < 9 ? defaultEpoch : lastEpoch);

						m_Window->SendMessage(SAM_DONEOPTIMIZING);

						std::this_thread::sleep_for(100ms);
					}
				}

				std::ostringstream resultOss;

				resultOss << std::fixed;

				for (std::size_t i = 0; i < trainData->size(); ++i) {
					if (i > 0) {
						resultOss << "\n\n";
					}

					const Matrix output = network.Forward((*trainData)[i].first);
					const float mse = lossFunction->Forward(output, (*trainData)[i].second);

					PrintInputOrOutput(resultOss, "�Է�", i, (*trainData)[i].first);
					resultOss << '\n';

					PrintInputOrOutput(resultOss, "����", i, (*trainData)[i].second);
					resultOss << '\n';

					PrintInputOrOutput(resultOss, "�н� �� ���", i, befores[i].first);
					resultOss << " (MSE " << befores[i].second << ")\n";

					PrintInputOrOutput(resultOss, "�н� �� ���", i, output);
					resultOss << " (MSE " << mse << ')';
				}

				m_Window->SendMessage(SAM_DONEOPTIMIZING, resultOss.str());
			});
		})));
	network->AddSubItem(MenuItemRef("��Ƽ������ ����", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			WindowDialogRef optimizerOptionDialog(*m_Window, "��Ƽ������ ����",
				std::make_unique<OptimizerOptionDialogHandler>(m_Project->GetNetwork()));

			if (optimizerOptionDialog->Show() == DialogResult::Ok) {
				m_IsSaved = false;

				UpdateText();
			}
		})));
	network->AddSubItem(MenuItemRef("�Ķ���� �ʱ�ȭ", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			// TODO
		})));

	network->AddSubItem(MenuItemSeparatorRef());
	network->AddSubItem(MenuItemRef("�������� �߰�", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			Network& network = m_Project->GetNetwork();
			const std::size_t layerCount = network.GetLayerCount();

			std::size_t inputSize = 0;

			if (layerCount > 0) {
				inputSize = network.GetOutputSize(layerCount - 1);
			}
			if (inputSize == 0) {
				const auto inputSizeTemp = AskInputOrOutputSize("�Է� ũ�� �Է� - �������� �߰�");

				if (!inputSizeTemp) return;

				inputSize = *inputSizeTemp;
			}

			const auto outputSize = AskInputOrOutputSize("��� ũ�� �Է� - �������� �߰�");

			if (!outputSize) return;

			m_Project->GetNetwork().AddLayer(std::make_unique<FCLayer>(inputSize, *outputSize));

			m_IsSaved = false;

			UpdateText();

			m_NetworkViewer->Invalidate();
		})));
	network->AddSubItem(MenuItemRef("Sigmoid Ȱ��ȭ�� �߰�", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			m_Project->GetNetwork().AddLayer(std::make_unique<ALayer>(AFunction::Sigmoid));

			m_IsSaved = false;

			UpdateText();

			m_NetworkViewer->Invalidate();
		})));
	network->AddSubItem(MenuItemRef("Tanh Ȱ��ȭ�� �߰�", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			m_Project->GetNetwork().AddLayer(std::make_unique<ALayer>(AFunction::Tanh));

			m_IsSaved = false;

			UpdateText();

			m_NetworkViewer->Invalidate();
		})));
	network->AddSubItem(MenuItemRef("ReLU Ȱ��ȭ�� �߰�", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			m_Project->GetNetwork().AddLayer(std::make_unique<ALayer>(AFunction::ReLU));

			m_IsSaved = false;

			UpdateText();

			m_NetworkViewer->Invalidate();
		})));
	network->AddSubItem(MenuItemRef("ReakyReLU Ȱ��ȭ�� �߰�", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			m_Project->GetNetwork().AddLayer(std::make_unique<ALayer>(AFunction::LeakyReLU));

			m_IsSaved = false;

			UpdateText();

			m_NetworkViewer->Invalidate();
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
std::optional<std::size_t> MainWindowHandler::AskInputOrOutputSize(std::string dialogTitle) {
	WindowDialogRef inputOrOutputSizeInputDialog(*m_Window, std::move(dialogTitle),
		std::make_unique<InputOrOutputSizeInputDialogHandler>());

	if (inputOrOutputSizeInputDialog->Show() == DialogResult::Ok)
		return dynamic_cast<InputOrOutputSizeInputDialogHandler&>(
			inputOrOutputSizeInputDialog->GetEventHandler()).GetInputOrOutputSize();
	else return std::nullopt;
}

void MainWindowHandler::StartOperation() {
	// TODO: �޴� ��Ȱ��ȭ
}
void MainWindowHandler::DoneOperation() {
	m_IsSaved = false;

	UpdateText();

	// TODO: �޴� Ȱ��ȭ
}
void MainWindowHandler::DoneTestOperation(std::string result) {
	DoneOperation();

	MessageDialogRef messageDialog(*m_Window, SAM_APPNAME, "�׽�Ʈ ���", std::move(result),
		MessageDialog::Information, MessageDialog::Ok);

	messageDialog->Show();
}
void MainWindowHandler::DoneFastOptimizingOperation(std::string result) {
	DoneOperation();

	m_NetworkViewer->Invalidate();

	MessageDialogRef messageDialog(*m_Window, SAM_APPNAME, "�н� ���", std::move(result),
		MessageDialog::Information, MessageDialog::Ok);

	messageDialog->Show();
}
void MainWindowHandler::DoneOptimizingOperation() {
	m_NetworkViewer->Invalidate();
}
void MainWindowHandler::DoneOptimizingOperation(std::string result) {
	DoneOperation();

	m_NetworkViewer->Invalidate();

	MessageDialogRef messageDialog(*m_Window, SAM_APPNAME, "�н� ���", std::move(result),
		MessageDialog::Information, MessageDialog::Ok);

	messageDialog->Show();
}