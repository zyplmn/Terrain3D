//==================================================================================================================|
// Created 2014.12.11 by Daniel L. Watkins
//
// Copyright (C) 2014 Daniel L. Watkins
// This file is licensed under the MIT License.
//==================================================================================================================|

#include "Settings.h"

#include <QtCore/QDir>
#include <QMetaEnum>

//TODO man, you need to unit test this

void Settings::init()
{
	initDefaultValues();
	checkForMissingMetaKeyInfoValues();

	QString filepath = QDir::currentPath()+"/Terrain3D.ini";
	mSettings = new QSettings(filepath, QSettings::IniFormat);
	mSettings->setValue("Version", mVersion);
	mSettings->sync();
}


void Settings::setValue(Key key, const QVariant &newValue)
{
	QString name = stringNameForKey(key);
	mSettings->setValue(name, newValue);
}


QVariant Settings::value(Key key)
{
	QVariant value = mSettings->value(stringNameForKey(key),
									  mMetaKeyInfo[key].defaultValue);

	if (QString(value.typeName()) == "QString" &&
		(value.toString() == "false" || value.toString() == "true"))
		return QVariant(value.toBool());

	return value;
}


void Settings::addListener(SettingsListener *listener)
{
	if (listener != nullptr  &&  !mListeners.contains(listener))
		mListeners.push_back(listener);
}


void Settings::removeListener(SettingsListener *listener)
{
	mListeners.removeOne(listener);
}


void Settings::applyQueuedValues()
{
	for (auto i : mSettingsQueue)
		setValue(i.first, i.second);

	mSettingsQueue.clear();
}


bool Settings::containsQueuedValueRequiringRestart()
{
	for (auto keyValuePair : mSettingsQueue)
	{
		if (mMetaKeyInfo[keyValuePair.first].requiresRestart)
			return true;
	}

	return false;
}


void Settings::enqueueValue(Key key, const QVariant &newValue)
{
	//verify the value is actually different than what is currently stored
	if (value(key) != newValue)
		mSettingsQueue.push_back(QPair<Key, QVariant>(key, newValue));
}


///// PRIVATE

QString Settings::stringNameForKey(Key key)
{
	const QMetaObject &mo = Settings::staticMetaObject;
	QMetaEnum me = mo.enumerator(mo.indexOfEnumerator("Key"));
	return me.valueToKey(key);
}


void Settings::initDefaultValues()
{
	#define d mMetaKeyInfo

	//graphics
	d[GraphicsScreenResolutionWidth] = {800, false};
	d[GraphicsScreenResolutionHeight] = {600, false};
	d[GraphicsScreenIsFullscreen] = {false, false};
	d[GraphicsCameraPositionX] = {0.0f, false};
	d[GraphicsCameraPositionY] = {0.0f, false};
	d[GraphicsCameraPositionZ] = {0.0f, false};
	d[GraphicsCameraFOV] = {50.0f, false};
	d[GraphicsCameraLOD] = {1.0f, false};
	d[GraphicsCameraWireframe] = {false, false};

	//world
	d[WorldGeneratorSize] = {128, true};
	d[WorldGeneratorTextureMapResolution] = {2, true};
	d[WorldGeneratorSeed] = {0, true};
	d[WorldTerrainSpacing] = {1.0f, false};
	d[WorldTerrainHeightScale] = {30.0f, false};
	d[WorldTerrainBlockSize] = {32, true};
	d[WorldTerrainSpanSize] = {8, false};

	#undef d
}


void Settings::checkForMissingMetaKeyInfoValues()
{
	const QMetaObject &mo = Settings::staticMetaObject;
	QMetaEnum me = mo.enumerator(mo.indexOfEnumerator("Key"));

	for (int i=0; i<me.keyCount(); i++)
	{
		Key key = static_cast<Key>(me.value(i));

		if (!mMetaKeyInfo.contains(key))
		{
			QString msg = (QString("Settings: No default value defined for key ")
							+ me.valueToKey(key) + " at:"
							+ QString(__FILE__) + " "
							+ QString(__func__));

			qFatal(msg.toStdString().c_str());
		}
	}
}