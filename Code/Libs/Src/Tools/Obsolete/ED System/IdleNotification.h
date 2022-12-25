// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef _h_IdleNotification
#define _h_IdleNotification
// --------------------------------------------------------------------------
class IdleNotification
{
	public:
		typedef void (*NotificationHandler) ();

		IdleNotification(const NotificationHandler handler);

		~IdleNotification();

		static void Notify();

	private:
		void NotifyObjects() const;

		static	IdleNotification*	mFirstNode;
		IdleNotification*			mNextNode;
		const NotificationHandler	mHandler;
};
// --------------------------------------------------------------------------
#endif