#include "ptyprocess.h"

#include <shv/coreqt/log.h>

#include <shv/core/utils.h>

#include <QSocketNotifier>

#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>

PtyProcess::PtyProcess(QObject *parent)
	: Super(parent)
{
	setProcessChannelMode(QProcess::ForwardedChannels);
	//if (ioctl(STDIN_FILENO, TIOCGWINSZ, &term_window_size) < 0)
	//		shvError() << "ioctl-TIOCGWINSZ";
}

PtyProcess::~PtyProcess()
{
	if(m_masterPtyFd > -1)
		::close(m_masterPtyFd);
}

void PtyProcess::openPty()
{
	m_masterPtyFd = ::posix_openpt(O_RDWR | O_NOCTTY | O_NONBLOCK); /* Open pty master */
	if (m_masterPtyFd == -1)
		throw std::runtime_error(strerror(errno));
	/* Grant access to slave pty */
	if (::grantpt(m_masterPtyFd) == -1) { /// BSD specific, not needed in Linux
		std::string err = "grantpt(): " + std::string(strerror(errno));
		::close(m_masterPtyFd);
		m_masterPtyFd = -1;
		throw std::runtime_error(err);
	}

	if (::unlockpt(m_masterPtyFd) == -1) { /* Unlock slave pty */
		std::string err = "unlockpt(): " + std::string(strerror(errno));
		::close(m_masterPtyFd);
		m_masterPtyFd = -1;
		throw std::runtime_error(err);
	}
	char *p = ptsname(m_masterPtyFd); /* Get slave pty name */
	if (p == NULL) {
		std::string err = "ptsname(): " + std::string(strerror(errno));
		::close(m_masterPtyFd);
		m_masterPtyFd = -1;
		throw std::runtime_error(err);
	}
	m_slavePtyName = p;
	/*
	/// create pipe to send slave pty fd back to parent process
	if (::pipe(m_sendSlavePtyFd) == -1) {
		std::string err = "pipe(): " + std::string(strerror(errno));
		::close(m_masterPtyFd);
		m_masterPtyFd = -1;
		throw std::runtime_error(err);
	}
	*/
	QSocketNotifier *master_pty_notifier = new QSocketNotifier(m_masterPtyFd, QSocketNotifier::Read, this);
	connect(master_pty_notifier, &QSocketNotifier::activated, this, &PtyProcess::readyReadMasterPty);

}

void PtyProcess::ptyStart(const std::string &cmd, int pty_cols, int pty_rows)
{
	m_ptyCols = pty_cols;
	m_ptyRows = pty_rows;
	openPty();
	start(QString::fromStdString(cmd));
	//::close(m_sendSlavePtyFd[1]);
}
/*
void PtyProcess::setTerminalWindowSize(int w, int h)
{
	m_ptyCols = w;
	m_ptyRows = h;
}
*/
qint64 PtyProcess::writePtyMaster(const char *data, int len)
{
	if(m_masterPtyFd == -1) {
		shvError() << "Attempt to write to closed master PTY.";
		return -1;
	}
	qint64 n = ::write(m_masterPtyFd, data, len);
	if(n != len) {
		shvError() << "Write master PTY error, only" << n << "of" << len << "bytes written.";
	}
	return n;
}

void PtyProcess::setupChildProcess()
{
	//if(0 != ::setpgid(0, ::getppid()))
	//	shvError() << "Error set process group ID:" << errno << ::strerror(errno);

	if (::setsid() == -1)  /* Start a new session */
		throw std::runtime_error("setsid(): " + std::string(::strerror(errno)));

	::close(m_masterPtyFd); /* Not needed in child */
	int slave_fd = ::open(m_slavePtyName.data(), O_RDWR | O_NONBLOCK); /* Becomes controlling tty */
	if (slave_fd == -1)
		throw std::runtime_error("open(\"" + m_slavePtyName + "\"): " + std::string(::strerror(errno)));
#ifdef TIOCSCTTY
	/* Acquire controlling tty on BSD */
	if (ioctl(slave_fd, TIOCSCTTY, 0) == -1)
		throw std::runtime_error("ioctl(slave_fd, TIOCSCTTY, 0): " + std::string(::strerror(errno)));
#endif
	/*
	if (slave_termios != NULL) // Set slave tty attributes
		if (tcsetattr(slave_fd, TCSANOW, slave_termios) == -1)
			err_exit("ptyFork:tcsetattr");
	if (pslave_WS != NULL) // Set slave tty window size
		if (ioctl(slave_fd, TIOCSWINSZ, pslave_WS) == -1)
			err_exit("ptyFork:ioctl-TIOCSWINSZ");
	*/
	if(m_ptyCols * m_ptyRows != 0) {
		struct winsize term_window_size;
		if (ioctl(slave_fd, TIOCGWINSZ, &term_window_size) == -1)
			throw std::runtime_error("ioctl(slave_fd, TIOCGWINSZ, &term_window_size): " + std::string(::strerror(errno)));
		term_window_size.ws_col = m_ptyCols;
		term_window_size.ws_row = m_ptyRows;
		if (ioctl(slave_fd, TIOCSWINSZ, &term_window_size) == -1)
			throw std::runtime_error("ioctl(slave_fd, TIOCSWINSZ, &term_window_size): " + std::string(::strerror(errno)));
	}
	/*
	::close(m_sendSlavePtyFd[0]);
	::write(m_sendSlavePtyFd[1], &slave_fd, sizeof(slave_fd);
	*/
	/* Duplicate pty slave to be child's stdin, stdout, and stderr */
	if (dup2(slave_fd, STDIN_FILENO) != STDIN_FILENO)
		throw std::runtime_error("ptyFork:dup2-STDIN_FILENO");
	if (dup2(slave_fd, STDOUT_FILENO) != STDOUT_FILENO)
		throw std::runtime_error("ptyFork:dup2-STDOUT_FILENO");
	if (dup2(slave_fd, STDERR_FILENO) != STDERR_FILENO)
		throw std::runtime_error("ptyFork:dup2-STDERR_FILENO");
	if (slave_fd > STDERR_FILENO) /* Safety check */
		::close(slave_fd);  /* No longer need this fd */
}
/*
int PtyProcess::slavePtyFd()
{
	if(m_slavePtyFd == -1) {
		::read(m_sendSlavePtyFd[0], &m_slavePtyFd, sizeof(m_slavePtyFd));
	}
	if(m_slavePtyFd == -1) {
		shvError() << "Read slave PTY fd from pipe failed";
	}
	return m_slavePtyFd;
}
*/
std::vector<char> PtyProcess::readAllMasterPty()
{
	return shv::core::Utils::readAllFd(m_masterPtyFd);
}

