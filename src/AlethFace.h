/*
	This file is part of cpp-ethereum.

	cpp-ethereum is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	cpp-ethereum is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with cpp-ethereum.  If not, see <http://www.gnu.org/licenses/>.
*/
/** @file AlethFace.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#pragma once

#include <memory>
#include <map>
#include <unordered_map>
#include <string>
#include <functional>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QAction>
#include <QtWidgets/QDockWidget>
#include <libevm/ExtVMFace.h>
#include "Context.h"
#include "Common.h"

class QSettings;

namespace dev
{

class WebThreeDirect;
class SafeHttpServer;
namespace eth { class Client; class LogFilter; class KeyManager; }
namespace shh { class WhisperHost; }

namespace aleth
{

#define DEV_AZ_NOTE_PLUGIN(ClassName) static DEV_UNUSED dev::aleth::PluginRegistrar<ClassName> s_notePlugin(#ClassName)

class Plugin;
class AlethFace;
class AlethZero;
class OurWebThreeStubServer;

using WatchHandler = std::function<void(dev::eth::LocalisedLogEntries const&)>;

class AccountNamer
{
	friend class Aleth;

public:
	virtual std::string toName(Address const&) const { return std::string(); }
	virtual Address toAddress(std::string const&) const { return Address(); }
	virtual Addresses knownAddresses() const { return Addresses(); }

protected:
	void noteKnownChanged();
	void noteNamesChanged();

private:
	AlethFace* m_main = nullptr;
};

using PluginFactory = std::function<Plugin*(AlethFace*)>;

class AlethFace: public QMainWindow, public Context
{
	Q_OBJECT

public:
	explicit AlethFace(QWidget* _parent = nullptr): QMainWindow(_parent) {}

	static void notePlugin(std::string const& _name, PluginFactory const& _new);
	static void unnotePlugin(std::string const& _name);

	void adoptPlugin(Plugin* _p);
	void killPlugins();

	void allChange();

	static Address getNameReg();
	static Address getICAPReg();

	// TODO: tidy - all should be references that throw if module unavailable.
	// TODO: provide a set of available web3 modules.
	virtual dev::WebThreeDirect* web3() const = 0;
	virtual dev::eth::Client* ethereum() const = 0;
	virtual OurWebThreeStubServer* web3Server() const = 0;
	virtual dev::SafeHttpServer* web3ServerConnector() const = 0;
	virtual std::shared_ptr<dev::shh::WhisperHost> whisper() const = 0;
	virtual NatSpecFace* natSpec() = 0;

	// Watch API
	virtual unsigned installWatch(dev::eth::LogFilter const& _tf, WatchHandler const& _f) = 0;
	virtual unsigned installWatch(dev::h256 const& _tf, WatchHandler const& _f) = 0;
	virtual void uninstallWatch(unsigned _id) = 0;

	// Account naming API
	virtual void install(AccountNamer* _adopt) = 0;
	virtual void uninstall(AccountNamer* _kill) = 0;
	virtual void noteKnownAddressesChanged(AccountNamer*) = 0;
	virtual void noteAddressNamesChanged(AccountNamer*) = 0;

	// Additional address lookup API
	std::string toReadable(Address const& _a) const;
	std::pair<Address, bytes> readAddress(std::string const& _n) const;
	std::string toHTML(eth::StateDiff const& _d) const;
	using Context::toHTML;

	// DNS lookup API
	std::string dnsLookup(std::string const& _a) const;

	// Key managing API
	virtual dev::Secret retrieveSecret(dev::Address const& _a) const = 0;
	virtual dev::eth::KeyManager& keyManager() = 0;
	virtual void noteKeysChanged() = 0;

	virtual void noteSettingsChanged() = 0;

protected:
	template <class F> void forEach(F const& _f) { for (auto const& p: m_plugins) _f(p.second); }
	std::shared_ptr<Plugin> takePlugin(std::string const& _name) { auto it = m_plugins.find(_name); std::shared_ptr<Plugin> ret; if (it != m_plugins.end()) { ret = it->second; m_plugins.erase(it); } return ret; }

	static std::unordered_map<std::string, PluginFactory>* s_linkedPlugins;

signals:
	void knownAddressesChanged();
	void addressNamesChanged();

	void keyManagerChanged();

private:
	std::unordered_map<std::string, std::shared_ptr<Plugin>> m_plugins;
};

class Plugin
{
public:
	Plugin(AlethFace* _f, std::string const& _name);
	virtual ~Plugin() {}

	std::string const& name() const { return m_name; }

	dev::WebThreeDirect* web3() const { return m_main->web3(); }
	dev::eth::Client* ethereum() const { return m_main->ethereum(); }
	std::shared_ptr<dev::shh::WhisperHost> whisper() const { return m_main->whisper(); }
	AlethFace* main() const { return m_main; }
	QDockWidget* dock(Qt::DockWidgetArea _area = Qt::RightDockWidgetArea, QString _title = QString());
	void addToDock(Qt::DockWidgetArea _area, QDockWidget* _dockwidget, Qt::Orientation _orientation);
	void addAction(QAction* _a);
	QAction* addMenuItem(QString _name, QString _menuName, bool _separate = false);

	virtual void onAllChange() {}
	virtual void readSettings(QSettings const&) {}
	virtual void writeSettings(QSettings&) {}

private:
	AlethFace* m_main = nullptr;
	std::string m_name;
	QDockWidget* m_dock = nullptr;
};

class AccountNamerPlugin: public Plugin, public AccountNamer
{
protected:
	AccountNamerPlugin(AlethFace* _m, std::string const& _name): Plugin(_m, _name) { main()->install(this); }
	~AccountNamerPlugin() { main()->uninstall(this); }
};

class PluginRegistrarBase
{
public:
	PluginRegistrarBase(std::string const& _name, PluginFactory const& _f);
	~PluginRegistrarBase();

private:
	std::string m_name;
};

template<class ClassName>
class PluginRegistrar: public PluginRegistrarBase
{
public:
	PluginRegistrar(std::string const& _name): PluginRegistrarBase(_name, [](AlethFace* m){ return new ClassName(m); }) {}
};

}

}
