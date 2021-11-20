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
				dynamic_cast<LearningRateInputDialogHandler&>(m_WindowDialog.GetEventHandler()).OnCancelButtonClick();
			}
		};

		m_CancelButton = &dynamic_cast<Button&>(dialog.AddChild(
			ButtonRef(std::make_unique<CancelButtonHandler>(*m_WindowDialog))));

		m_CancelButton->SetText("취소");
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
			MessageDialogRef messageDialog(m_WindowDialog->GetWindow(), SAM_APPNAME, "올바르지 않은 형식입니다",
				"학습률을 입력했는지 확인해 보세요.",
				MessageDialog::Error, MessageDialog::Ok);

			messageDialog->Show();
		} else if (iss.fail() || iss.bad() || learningRate <= 0 || learningRate > 1) {
			MessageDialogRef messageDialog(m_WindowDialog->GetWindow(), SAM_APPNAME, "올바르지 않은 형식입니다",
				"학습률이 0 초과 1 미만의 실수인지 확인해 보세요.",
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
				dynamic_cast<EpochInputDialogHandler&>(m_WindowDialog.GetEventHandler()).OnCancelButtonClick();
			}
		};

		m_CancelButton = &dynamic_cast<Button&>(dialog.AddChild(
			ButtonRef(std::make_unique<CancelButtonHandler>(*m_WindowDialog))));

		m_CancelButton->SetText("취소");
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
			MessageDialogRef messageDialog(m_WindowDialog->GetWindow(), SAM_APPNAME, "올바르지 않은 형식입니다",
				"에포크를 입력했는지 확인해 보세요.",
				MessageDialog::Error, MessageDialog::Ok);

			messageDialog->Show();
		} else if (iss.fail() || iss.bad() || epoch == 0) {
			MessageDialogRef messageDialog(m_WindowDialog->GetWindow(), SAM_APPNAME, "올바르지 않은 형식입니다",
				"에포크가 자연수인지 확인해 보세요.",
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

		m_OptimizerNameComboBox->AddItem("확률적 경사 하강법");

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

		m_OkButton->SetText("확인");
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

		m_ApplyButton->SetText("적용");
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

		m_CancelButton->SetText("취소");
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
			MessageDialogRef messageDialog(m_WindowDialog->GetWindow(), SAM_APPNAME, "손실 함수가 선택되지 않았습니다", {},
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
			MessageDialogRef messageDialog(m_WindowDialog->GetWindow(), SAM_APPNAME, "올바르지 않은 형식입니다",
				"학습률을 입력했는지 확인해 보세요.",
				MessageDialog::Error, MessageDialog::Ok);

			messageDialog->Show();

			return std::nullopt;
		} else if (iss.fail() || iss.bad() || learningRate <= 0 || learningRate > 1) {
			MessageDialogRef messageDialog(m_WindowDialog->GetWindow(), SAM_APPNAME, "올바르지 않은 형식입니다",
				"학습률이 0 초과 1 이하의 실수인지 확인해 보세요.",
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
				dynamic_cast<InputOrOutputSizeInputDialogHandler&>(
					m_WindowDialog.GetEventHandler()).OnCancelButtonClick();
			}
		};

		m_CancelButton = &dynamic_cast<Button&>(dialog.AddChild(
			ButtonRef(std::make_unique<CancelButtonHandler>(*m_WindowDialog))));

		m_CancelButton->SetText("취소");
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
			MessageDialogRef messageDialog(m_WindowDialog->GetWindow(), SAM_APPNAME, "올바르지 않은 형식입니다",
				"크기를 입력했는지 확인해 보세요.",
				MessageDialog::Error, MessageDialog::Ok);

			messageDialog->Show();
		} else if (iss.fail() || iss.bad() || inputOrOutputSize == 0) {
			MessageDialogRef messageDialog(m_WindowDialog->GetWindow(), SAM_APPNAME, "올바르지 않은 형식입니다",
				"크기가 자연수인지 확인해 보세요.",
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

	network->AddSubItem(MenuItemRef("테스트", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			const auto trainData = AskTrainData("테스트 데이터 입력 - 실행");

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

					PrintInputOrOutput(resultOss, "입력", i, (*trainData)[i].first);
					resultOss << '\n';

					PrintInputOrOutput(resultOss, "정답", i, (*trainData)[i].second);
					resultOss << '\n';

					PrintInputOrOutput(resultOss, "출력", i, output);
					resultOss << " (MSE " << mse << ')';
				}

				m_Window->SendMessage(SAM_DONETEST, resultOss.str());
			});
		})));

	network->AddSubItem(MenuItemSeparatorRef());
	network->AddSubItem(MenuItemRef("빠른 학습", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			if (!m_Project->GetNetwork().HasOptimizer()) {
				MessageDialogRef messageDialog(*m_Window, SAM_APPNAME, "옵티마이저가 없습니다",
					"옵티마이저를 설정했는지 확인해 보세요.",
					MessageDialog::Error, MessageDialog::Ok);

				messageDialog->Show();

				return;
			}

			const auto trainData = AskTrainData("학습 데이터 입력 - 빠른 학습");

			if (!trainData) return;

			const auto epoch = AskEpoch("에포크 입력 - 빠른 학습");

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

					PrintInputOrOutput(resultOss, "입력", i, (*trainData)[i].first);
					resultOss << '\n';

					PrintInputOrOutput(resultOss, "정답", i, (*trainData)[i].second);
					resultOss << '\n';

					PrintInputOrOutput(resultOss, "학습 전 출력", i, befores[i].first);
					resultOss << " (MSE " << befores[i].second << ")\n";

					PrintInputOrOutput(resultOss, "학습 후 출력", i, output);
					resultOss << " (MSE " << mse << ')';
				}

				m_Window->SendMessage(SAM_DONEFASTOPTIMIZING, resultOss.str());
			});
		})));
	network->AddSubItem(MenuItemRef("학습 및 시각화", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			if (!m_Project->GetNetwork().HasOptimizer()) {
				MessageDialogRef messageDialog(*m_Window, SAM_APPNAME, "옵티마이저가 없습니다",
					"옵티마이저를 설정했는지 확인해 보세요.",
					MessageDialog::Error, MessageDialog::Ok);

				messageDialog->Show();

				return;
			}

			const auto trainData = AskTrainData("학습 데이터 입력 - 학습 및 시각화");

			if (!trainData) return;

			const auto epoch = AskEpoch("에포크 입력 - 학습 및 시각화");

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

					PrintInputOrOutput(resultOss, "입력", i, (*trainData)[i].first);
					resultOss << '\n';

					PrintInputOrOutput(resultOss, "정답", i, (*trainData)[i].second);
					resultOss << '\n';

					PrintInputOrOutput(resultOss, "학습 전 출력", i, befores[i].first);
					resultOss << " (MSE " << befores[i].second << ")\n";

					PrintInputOrOutput(resultOss, "학습 후 출력", i, output);
					resultOss << " (MSE " << mse << ')';
				}

				m_Window->SendMessage(SAM_DONEOPTIMIZING, resultOss.str());
			});
		})));
	network->AddSubItem(MenuItemRef("옵티마이저 설정", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			WindowDialogRef optimizerOptionDialog(*m_Window, "옵티마이저 설정",
				std::make_unique<OptimizerOptionDialogHandler>(m_Project->GetNetwork()));

			if (optimizerOptionDialog->Show() == DialogResult::Ok) {
				m_IsSaved = false;

				UpdateText();
			}
		})));
	network->AddSubItem(MenuItemRef("파라미터 초기화", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			// TODO
		})));

	network->AddSubItem(MenuItemSeparatorRef());
	network->AddSubItem(MenuItemRef("전결합층 추가", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			Network& network = m_Project->GetNetwork();
			const std::size_t layerCount = network.GetLayerCount();

			std::size_t inputSize = 0;

			if (layerCount > 0) {
				inputSize = network.GetOutputSize(layerCount - 1);
			}
			if (inputSize == 0) {
				const auto inputSizeTemp = AskInputOrOutputSize("입력 크기 입력 - 전결합층 추가");

				if (!inputSizeTemp) return;

				inputSize = *inputSizeTemp;
			}

			const auto outputSize = AskInputOrOutputSize("출력 크기 입력 - 전결합층 추가");

			if (!outputSize) return;

			m_Project->GetNetwork().AddLayer(std::make_unique<FCLayer>(inputSize, *outputSize));

			m_IsSaved = false;

			UpdateText();

			m_NetworkViewer->Invalidate();
		})));
	network->AddSubItem(MenuItemRef("Sigmoid 활성화층 추가", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			m_Project->GetNetwork().AddLayer(std::make_unique<ALayer>(AFunction::Sigmoid));

			m_IsSaved = false;

			UpdateText();

			m_NetworkViewer->Invalidate();
		})));
	network->AddSubItem(MenuItemRef("Tanh 활성화층 추가", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			m_Project->GetNetwork().AddLayer(std::make_unique<ALayer>(AFunction::Tanh));

			m_IsSaved = false;

			UpdateText();

			m_NetworkViewer->Invalidate();
		})));
	network->AddSubItem(MenuItemRef("ReLU 활성화층 추가", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			m_Project->GetNetwork().AddLayer(std::make_unique<ALayer>(AFunction::ReLU));

			m_IsSaved = false;

			UpdateText();

			m_NetworkViewer->Invalidate();
		})));
	network->AddSubItem(MenuItemRef("ReakyReLU 활성화층 추가", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			m_Project->GetNetwork().AddLayer(std::make_unique<ALayer>(AFunction::LeakyReLU));

			m_IsSaved = false;

			UpdateText();

			m_NetworkViewer->Invalidate();
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
	// TODO: 메뉴 비활성화
}
void MainWindowHandler::DoneOperation() {
	m_IsSaved = false;

	UpdateText();

	// TODO: 메뉴 활성화
}
void MainWindowHandler::DoneTestOperation(std::string result) {
	DoneOperation();

	MessageDialogRef messageDialog(*m_Window, SAM_APPNAME, "테스트 결과", std::move(result),
		MessageDialog::Information, MessageDialog::Ok);

	messageDialog->Show();
}
void MainWindowHandler::DoneFastOptimizingOperation(std::string result) {
	DoneOperation();

	m_NetworkViewer->Invalidate();

	MessageDialogRef messageDialog(*m_Window, SAM_APPNAME, "학습 결과", std::move(result),
		MessageDialog::Information, MessageDialog::Ok);

	messageDialog->Show();
}
void MainWindowHandler::DoneOptimizingOperation() {
	m_NetworkViewer->Invalidate();
}
void MainWindowHandler::DoneOptimizingOperation(std::string result) {
	DoneOperation();

	m_NetworkViewer->Invalidate();

	MessageDialogRef messageDialog(*m_Window, SAM_APPNAME, "학습 결과", std::move(result),
		MessageDialog::Information, MessageDialog::Ok);

	messageDialog->Show();
}