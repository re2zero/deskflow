/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "NetworkMonitor.h"

#include <QNetworkInterface>
#include <QTimer>
#include <QDebug>

namespace deskflow::gui::core::network {

NetworkMonitor::NetworkMonitor(QObject *parent)
    : QObject(parent)
    , m_checkTimer(new QTimer(this))
    , m_isMonitoring(false)
{
  // Setup periodic check timer
  connect(m_checkTimer, SIGNAL(timeout()), this, SLOT(checkNetworkState()));
}

void NetworkMonitor::startMonitoring(int intervalMs)
{
  if (m_isMonitoring) {
    return;
  }

  // Initial update
  updateNetworkState();

  // Start periodic checks
  m_checkTimer->start(intervalMs);
  m_isMonitoring = true;
}

void NetworkMonitor::stopMonitoring()
{
  if (!m_isMonitoring) {
    return;
  }

  m_checkTimer->stop();
  m_isMonitoring = false;
}

void NetworkMonitor::refreshNetwork()
{
  // Force an immediate network state update
  if (updateNetworkState()) {
    Q_EMIT networkConfigurationChanged();
  }
}

QVector<QHostAddress> NetworkMonitor::getAvailableIPv4Addresses() const
{
  QVector<QHostAddress> addresses;
  
  for (const auto &interface : QNetworkInterface::allInterfaces()) {
    if (interface.flags() & QNetworkInterface::IsUp && 
        interface.flags() & QNetworkInterface::IsRunning &&
        !(interface.flags() & QNetworkInterface::IsLoopBack)) {
      
      for (const auto &address : interface.allAddresses()) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol && 
            address != QHostAddress(QHostAddress::LocalHost) &&
            !address.isInSubnet(QHostAddress::parseSubnet("169.254.0.0/16"))) {
          addresses.append(address);
        }
      }
    }
  }
  
  return addresses;
}

QHostAddress NetworkMonitor::getSuggestedIPv4Address() const
{
  // If we have a selected IP address, prefer it
  if (!m_selectedIPAddress.isNull()) {
    const auto availableAddresses = getAvailableIPv4Addresses();
    for (const auto &address : availableAddresses) {
      if (address == m_selectedIPAddress) {
        return address;
      }
    }
    // If the selected IP is no longer available, fall back to the default logic
  }
  
  const auto addresses = getAvailableIPv4Addresses();
  
  // Prefer 192.168.x.x addresses as they are commonly used in home/office networks
  for (const auto &address : addresses) {
    if (address.isInSubnet(QHostAddress::parseSubnet("192.168/16"))) {
      return address;
    }
  }
  
  // If no 192.168.x.x address found, return the first available address
  if (!addresses.isEmpty()) {
    return addresses.first();
  }
  
  // Prefer 192.168.x.x addresses as they are commonly used in home/office networks
  for (const auto &address : addresses) {
    if (address.isInSubnet(QHostAddress::parseSubnet("192.168/16"))) {
      return address;
    }
  }
  
  // If no 192.168.x.x address found, return the first available address
  if (!addresses.isEmpty()) {
    return addresses.first();
  }
  
  return QHostAddress(); // Return null address if no suitable IP found
}

bool NetworkMonitor::hasNetworkChanged() const
{
  const auto currentAddresses = getAvailableIPv4Addresses();
  
  if (currentAddresses.size() != m_lastAddresses.size()) {
    return true;
  }
  
  for (const auto &address : currentAddresses) {
    if (!m_lastAddresses.contains(address)) {
      return true;
    }
  }
  
  // Check if the selected IP is still available
  if (!m_selectedIPAddress.isNull()) {
    bool found = false;
    for (const auto &address : currentAddresses) {
      if (address == m_selectedIPAddress) {
        found = true;
        break;
      }
    }
    if (!found) {
      return true;
    }
  }
  
  return false;
}

bool NetworkMonitor::hasMultipleActiveNetworks() const
{
  const auto activeInterfaces = getActiveNetworkInterfaces();
  return activeInterfaces.size() > 1;
}

void NetworkMonitor::checkNetworkState()
{
  if (updateNetworkState()) {
    Q_EMIT networkConfigurationChanged();
    Q_EMIT ipAddressesChanged(m_lastAddresses);
    
    if (hasMultipleActiveNetworks()) {
      Q_EMIT multipleNetworksDetected(getActiveNetworkInterfaces());
    }
  }
}

bool NetworkMonitor::updateNetworkState()
{
  const auto currentAddresses = getAvailableIPv4Addresses();
  const auto currentInterfaces = getActiveNetworkInterfaces();
  
  bool addressesChanged = (currentAddresses.size() != m_lastAddresses.size());
  if (!addressesChanged) {
    for (const auto &address : currentAddresses) {
      if (!m_lastAddresses.contains(address)) {
        addressesChanged = true;
        break;
      }
    }
  }
  
  bool interfacesChanged = (currentInterfaces.size() != m_lastNetworkInterfaces.size());
  if (!interfacesChanged) {
    for (const auto &interface : currentInterfaces) {
      bool found = false;
      for (const auto &lastInterface : m_lastNetworkInterfaces) {
        if (interface.name() == lastInterface.name()) {
          found = true;
          break;
        }
      }
      if (!found) {
        interfacesChanged = true;
        break;
      }
    }
  }
  
  // Check if the selected IP is still available
  bool selectedIPChanged = false;
  if (!m_selectedIPAddress.isNull()) {
    bool found = false;
    for (const auto &address : currentAddresses) {
      if (address == m_selectedIPAddress) {
        found = true;
        break;
      }
    }
    selectedIPChanged = !found;
  }
  
  if (addressesChanged || interfacesChanged || selectedIPChanged) {
    m_lastAddresses = currentAddresses;
    m_lastActiveInterfaces.clear();
    for (const auto &interface : currentInterfaces) {
      m_lastActiveInterfaces.append(interface.humanReadableName());
    }
    m_lastNetworkInterfaces = currentInterfaces;
    return true;
  }
  
  return false;
}

QList<QNetworkInterface> NetworkMonitor::getActiveNetworkInterfaces() const
{
  QList<QNetworkInterface> interfaces;
  
  for (const auto &interface : QNetworkInterface::allInterfaces()) {
    if (interface.flags() & QNetworkInterface::IsUp && 
        interface.flags() & QNetworkInterface::IsRunning &&
        !(interface.flags() & QNetworkInterface::IsLoopBack)) {
      interfaces.append(interface);
    }
  }
  
  return interfaces;
}

void NetworkMonitor::setSelectedIPAddress(const QHostAddress &address)
{
  m_selectedIPAddress = address;
}

QHostAddress NetworkMonitor::selectedIPAddress() const
{
  return m_selectedIPAddress;
}

} // namespace deskflow::gui::core::network