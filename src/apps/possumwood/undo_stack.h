#pragma once

#include <functional>
#include <vector>

#include <boost/noncopyable.hpp>

/// A simple implementation of undo stack based on Command design pattern.
/// Each command uses a vector of std::functors to store the actual code
/// to be executed on each undo and redo.
class UndoStack : public boost::noncopyable {
	public:
		/// Action implementation - stores a stack of "things to be done".
		/// These are not executed on construction, but only after pushing onto
		/// a stack. Each action is atomic - if an exception is thrown while
		/// executing an action, all commands of a failed action are rolled back.
		class Action {
			public:
				void addCommand(const std::function<void()>& redo, const std::function<void()>& undo);

			private:
				std::vector<std::function<void()>> m_redo, m_undo;

			/// the actual implementation is handled in UndoStack code.
			friend class UndoStack;
		};

		/// Executes an action and puts it to the undo stack to make it undoable.
		/// Any potential exceptions thrown from an action are handled (assuming
		/// they are based on std::exception) - an error during a command will
		/// assume that command has not been finished (and any remaining state
		/// is rolled back), and all previously finished commands of an action
		/// needs rolling back. Exception is then forwared to the caller.
		void execute(const Action& action);

		/// Undoes last succesfully executed action. Cannot throw - action worked
		/// last time, so it should work this time as well.
		void undo();
		/// Redoes last undone action. The redo queue gets cleared when a new action
		/// is inserted onto the stack.
		void redo();

	protected:
	private:
		std::vector<Action> m_undoStack, m_redoStack;
};
