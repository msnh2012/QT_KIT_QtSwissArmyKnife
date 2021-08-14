﻿/****************************************************************************************
 * Copyright 2018-2021 Qter(qsaker@qq.com). All rights reserved.
 *
 * The file is encoded using "utf8 with bom", it is a part
 * of QtSwissArmyKnife project.
 *
 * QtSwissArmyKnife is licensed according to the terms in
 * the file LICENCE in the root of the source code directory.
 ***************************************************************************************/
#include <QDebug>
#include <QDateTime>
#include <QRegularExpression>
#include <QRegularExpressionValidator>

#include "SAKDebugger.hh"
#include "SAKCommonInterface.hh"
#include "SAKCommonDataStructure.hh"
#include "SAKDebuggerPluginAutoResponseItem.hh"
#include "ui_SAKDebuggerPluginAutoResponseItem.h"

SAKDebuggerPluginAutoResponseItem::SAKDebuggerPluginAutoResponseItem(QWidget *parent)
    :QWidget(parent)
    ,mUi(new Ui::SAKDebuggerPluginAutoResponseItem)
{
    mUi->setupUi(this);
    mID = QDateTime::currentMSecsSinceEpoch();
    setupItem();
}

SAKDebuggerPluginAutoResponseItem::SAKDebuggerPluginAutoResponseItem(
        SAKStructItemContext ctx,
        QWidget *parent)
    :QWidget(parent)
    ,mID(ctx.id)
    ,mUi(new Ui::SAKDebuggerPluginAutoResponseItem)
{
    mUi->setupUi(this);
    mUi->descriptionLineEdit->setText(ctx.name);
    mUi->referenceLineEdit->setText(ctx.referenceData);
    mUi->responseLineEdit->setText(ctx.responseData);
    mUi->enableCheckBox->setChecked(ctx.enable);
    mUi->referenceDataFromatComboBox->setCurrentIndex(ctx.referenceFormat);
    mUi->responseDataFormatComboBox->setCurrentIndex(ctx.responseFormat);
    mUi->optionComboBox->setCurrentIndex(ctx.option);
    mUi->delayResponseCheckBox->setCheckable(ctx.delay);
    mUi->delayResponseSpinBox->setValue(ctx.interval);

    setupItem();
}

SAKDebuggerPluginAutoResponseItem::~SAKDebuggerPluginAutoResponseItem()
{
    delete mUi;
}

SAKDebuggerPluginAutoResponseItem::SAKStructItemContext
SAKDebuggerPluginAutoResponseItem::context()
{
    SAKStructItemContext ctx;
    ctx.id = mID;
    ctx.name = mUi->descriptionLineEdit->text();
    ctx.delay = mUi->delayResponseCheckBox->isChecked();
    ctx.enable = mUi->enableCheckBox->isChecked();
    ctx.option = mUi->optionComboBox->currentIndex();
    ctx.interval = mUi->delayResponseSpinBox->value();
    ctx.referenceData = mUi->referenceLineEdit->text();
    ctx.responseData = mUi->responseLineEdit->text();
    ctx.responseFormat = mUi->responseDataFormatComboBox->currentData().toInt();
    ctx.referenceFormat = mUi->referenceDataFromatComboBox->currentData().toInt();

    return ctx;
}

void SAKDebuggerPluginAutoResponseItem::onBytesRead(const QByteArray &bytes)
{
    if (bytes.isEmpty() || (!mUi->enableCheckBox->isChecked())) {
        return;
    }


    QString referenceString;
    int referenceFormat = mUi->referenceDataFromatComboBox->currentData().toInt();
    if ((referenceFormat == SAKCommonDataStructure::InputFormatBin) ||
            (referenceFormat == SAKCommonDataStructure::InputFormatOct) ||
            (referenceFormat == SAKCommonDataStructure::InputFormatDec) ||
            (referenceFormat == SAKCommonDataStructure::InputFormatHex)) {
        referenceString = mUi->referenceLineEdit->text().trimmed();
    } else {
        referenceString = mUi->referenceLineEdit->text();
    }


    auto cookedReferenceFormat =
            static_cast<SAKCommonDataStructure::SAKEnumTextFormatInput>(referenceFormat);
    QByteArray referenceData =
            SAKCommonDataStructure::stringToByteArray(referenceString,
                                                      cookedReferenceFormat);
    if (response(bytes, referenceData, mUi->optionComboBox->currentData().toInt())) {
         QString responseString = mUi->responseLineEdit->text();
         int responseFromat = mUi->referenceDataFromatComboBox->currentData().toInt();
         auto cookedResponseFromat =
                 static_cast<SAKCommonDataStructure::SAKEnumTextFormatInput>(
                     responseFromat
                     );
         QByteArray responseData = SAKCommonDataStructure::stringToByteArray(
                     responseString,
                     cookedResponseFromat
                     );


         if (!responseData.isEmpty()) {
             if (mUi->delayResponseCheckBox->isChecked()) {
                 int delay = mUi->delayResponseSpinBox->value();
                 QTimer::singleShot(delay, this, [=](){
                     emit responseBytes(responseData);
                 });
             } else {
                 emit responseBytes(responseData);
             }
         }
    }
}

void SAKDebuggerPluginAutoResponseItem::setupItem()
{
    mUi->optionComboBox->clear();
    int dataValue = ReadDataIsEqualToReference;
    mUi->optionComboBox->addItem(tr("Rx data is equal to reference data"),
                                 QVariant::fromValue<int>(dataValue));
    dataValue = ReadDataContainsReferenceData;
    mUi->optionComboBox->addItem(tr("Rx data contains reference data"),
                                 QVariant::fromValue<int>(dataValue));
    dataValue = ReadDataDoesNotContainReferenceData;
    mUi->optionComboBox->addItem(tr("Rx data does not contains reference data"),
                                 QVariant::fromValue<int>(dataValue));

    SAKCommonDataStructure::setComboBoxTextInputFormat(
                mUi->referenceDataFromatComboBox
                );
    SAKCommonDataStructure::setComboBoxTextInputFormat(
                mUi->responseDataFormatComboBox
                );


    connect(mUi->descriptionLineEdit, &QLineEdit::textChanged,
            this, [&](const QString description){
        emit descriptionChanged(description);
    });

    connect(mUi->referenceLineEdit, &QLineEdit::textChanged,
            this, [&](const QString description){
        emit referenceTextChanged(description);
    });

    connect(mUi->responseLineEdit, &QLineEdit::textChanged,
            this, [&](const QString description){
        emit responseTextChanged(description);
    });


    connect(mUi->optionComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [&](int option){
        emit optionChanged(option);
    });


    connect(mUi->referenceDataFromatComboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [&](int format){
        emit referenceFormatChanged(format);
    });

    connect(mUi->responseDataFormatComboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [&](int format){
        emit responseFromatChanged(format);
    });


    connect(mUi->delayResponseCheckBox, &QCheckBox::clicked,
            this, [&](){
        emit delayChanged(mUi->delayResponseCheckBox->isChecked());
    });


    connect(mUi->delayResponseSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [&](int interval){
        emit intervalChanged(interval);
    });
}

bool SAKDebuggerPluginAutoResponseItem::response(QByteArray receiveData,
                                                  QByteArray referenceData,
                                                  int option)
{
    if (option == ReadDataIsEqualToReference){
        return (QString(receiveData.toHex()) == QString(referenceData.toHex()));
    }

    if (option == ReadDataContainsReferenceData){
        return (QString(receiveData.toHex()).contains(QString(referenceData.toHex())));
    }

    if (option == ReadDataDoesNotContainReferenceData){
        return !(QString(receiveData.toHex()).contains(QString(referenceData.toHex())));
    }

    return false;
};
