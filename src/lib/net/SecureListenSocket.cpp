/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2024 Deskflow Developers
 * Copyright (C) 2015-2016 Symless Ltd.
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "SecureListenSocket.h"

#include "SecureSocket.h"
#include "arch/XArch.h"
#include "base/String.h"
#include "common/constants.h"
#include "deskflow/ArgParser.h"
#include "deskflow/ArgsBase.h"
#include "net/NetworkAddress.h"
#include "net/SocketMultiplexer.h"
#include "net/TSocketMultiplexerMethodJob.h"

// TODO: Reduce duplication of these strings between here and SecureSocket.cpp
static const char s_certificateDir[] = {"tls"};
static const char s_certificateFileExt[] = {"pem"};

//
// SecureListenSocket
//

SecureListenSocket::SecureListenSocket(
    IEventQueue *events, SocketMultiplexer *socketMultiplexer, IArchNetwork::EAddressFamily family,
    SecurityLevel securityLevel
)
    : TCPListenSocket(events, socketMultiplexer, family),
      m_securityLevel{securityLevel}

{
}

IDataSocket *SecureListenSocket::accept()
{
  SecureSocket *socket = NULL;
  try {
    socket = new SecureSocket(m_events, m_socketMultiplexer, ARCH->acceptSocket(m_socket, NULL), m_securityLevel);
    socket->initSsl(true);

    if (socket != NULL) {
      setListeningJob();
    }

    // default location of the TLS cert file in users dir
    std::string certificateFilename = deskflow::string::sprintf(
        "%s/%s/%s.%s", ARCH->getProfileDirectory().c_str(), s_certificateDir, kAppId, s_certificateFileExt
    );

    // if the tls cert option is set use that for the certificate file
    if (!ArgParser::argsBase().m_tlsCertFile.empty()) {
      certificateFilename = ArgParser::argsBase().m_tlsCertFile;
    }

    bool loaded = socket->loadCertificates(certificateFilename);
    if (!loaded) {
      delete socket;
      return NULL;
    }

    socket->secureAccept();

    return dynamic_cast<IDataSocket *>(socket);
  } catch (XArchNetwork &) {
    if (socket != NULL) {
      delete socket;
      setListeningJob();
    }
    return NULL;
  } catch (std::exception &ex) {
    if (socket != NULL) {
      delete socket;
      setListeningJob();
    }
    throw ex;
  }
}
