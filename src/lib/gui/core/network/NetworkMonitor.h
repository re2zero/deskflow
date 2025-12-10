/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QHostAddress>
#include <QNetworkInterface>
#include <QObject>
#include <QTimer>
#include <QVector>
#include <QStringList>

namespace deskflow::gui::core::network {

/**
 * @brief 监控网络活动变化并提供IP地址更新功能
 */
class NetworkMonitor : public QObject
{
  Q_OBJECT

public:
  explicit NetworkMonitor(QObject *parent = nullptr);
  ~NetworkMonitor() override = default;

  /**
   * @brief 启动网络监控
   * @param intervalMs 检查间隔(毫秒)，默认3000ms(3秒)
   */
  void startMonitoring(int intervalMs = 3000);

  /**
   * @brief 停止网络监控
   */
  void stopMonitoring();

  /**
   * @brief 手动刷新网络状态
   */
  void refreshNetwork();

  /**
   * @brief 获取所有可用的IPv4地址列表(排除本地和链路本地地址)
   * @return IPv4地址列表
   */
  QVector<QHostAddress> getAvailableIPv4Addresses() const;

  /**
   * @brief 获取推荐的IP地址(192.168.x.x优先)
   * @return 推荐的IP地址，如果没有则返回空
   */
  QHostAddress getSuggestedIPv4Address() const;

  /**
   * @brief 检查网络是否有变化
   * @return true如果有新的网络接口或IP地址变化
   */
  bool hasNetworkChanged() const;

  /**
   * @brief 检查是否有多个活动网络接口
   * @return true如果有多个活动网络接口(例如有线和无线同时连接)
   */
  bool hasMultipleActiveNetworks() const;

  /**
   * @brief 设置选择的IP地址
   * @param address 选择的IP地址
   */
  void setSelectedIPAddress(const QHostAddress &address);
  
  /**
   * @brief 获取选择的IP地址
   * @return 选择的IP地址，如果没有选择则返回空地址
   */
  QHostAddress selectedIPAddress() const;

Q_SIGNALS:
  /**
   * @brief 当网络配置发生变化时发出
   */
  void networkConfigurationChanged();

  /**
   * @brief 当IP地址发生变化时发出
   * @param addresses 新的IP地址列表
   */
  void ipAddressesChanged(const QVector<QHostAddress> &addresses);

  /**
   * @brief 当检测到多个活动网络接口时发出
   * @param interfaces 网络接口名称列表
   */
  void multipleNetworksDetected(const QList<QNetworkInterface> &interfaces);

private Q_SLOTS:
  /**
   * @brief 定期检查网络状态
   */
  void checkNetworkState();

private:
  /**
   * @brief 更新当前网络状态
   * @return true如果网络状态有变化
   */
  bool updateNetworkState();

  /**
   * @brief 获取活动网络接口列表
   * @return 活动网络接口列表
   */
  QList<QNetworkInterface> getActiveNetworkInterfaces() const;
  


  QTimer *m_checkTimer;
  QVector<QHostAddress> m_lastAddresses;
  QStringList m_lastActiveInterfaces;
  QList<QNetworkInterface> m_lastNetworkInterfaces;
  QHostAddress m_selectedIPAddress;
  bool m_isMonitoring;
};

} // namespace deskflow::gui::core::network