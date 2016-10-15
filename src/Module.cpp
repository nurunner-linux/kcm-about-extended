/*
  Copyright (C) 2012-2014 Harald Sitter <apachelogger@ubuntu.com>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of
  the License or (at your option) version 3 or any later version
  accepted by the membership of KDE e.V. (or its successor approved
  by the membership of KDE e.V.), which shall act as a proxy
  defined in Section 14 of version 3 of the license.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Module.h"
#include "ui_Module.h"
#include "openglinfo.h"

#include <QIcon>
#include <QStandardPaths>
#include <QDebug>

#include <KAboutData>
#include <KConfig>
#include <KConfigGroup>
#include <KDesktopFile>
#include <KFormat>
#include <KLocalizedString>
#include <KSharedConfig>
#include <KCoreAddons>

#include <Solid/Device>
#include <Solid/Processor>
#include <NetworkManagerQt/Manager>

#ifdef Q_OS_LINUX
#include <sys/sysinfo.h>
#endif
#include <sys/utsname.h>

#include "OSRelease.h"
#include "Version.h"

static qlonglong calculateTotalRam()
{
    qlonglong ret = -1;
#ifdef Q_OS_LINUX
    struct sysinfo info;
    if (sysinfo(&info) == 0)
        // manpage "sizes are given as multiples of mem_unit bytes"
        ret = info.totalram * info.mem_unit;
#endif
    return ret;
}

static qlonglong calculateTotalSwap()
{
    qlonglong ret = -1;
#ifdef Q_OS_LINUX
    struct sysinfo info;
    if (sysinfo(&info) == 0)
        // manpage "sizes are given as multiples of mem_unit bytes"
        ret = info.totalswap * info.mem_unit;
#endif
    return ret;
}


Module::Module(QWidget *parent, const QVariantList &args) :
    KCModule(parent, args),
    ui(new Ui::Module)
{
    KAboutData *aboutData = new KAboutData(QStringLiteral("kcm-about-extended"),
                                           i18nc("@title", "About Distribution"),
                                           global_s_versionStringFull,
                                           QStringLiteral(""),
                                           KAboutLicense::LicenseKey::GPL_V3,
                                           i18nc("@info:credit", "Copyright 2012-2014 Harald Sitter"));

    aboutData->addAuthor(i18nc("@info:credit", "Harald Sitter"),
                         i18nc("@info:credit", "Author"),
                         QStringLiteral("apachelogger@kubuntu.org"));

    aboutData->addAuthor(i18nc("@info:credit", "Rohan Garg"),
                         i18nc("@info:credit", "Author"),
                         QStringLiteral("rohan@kde.org"));

    setAboutData(aboutData);

    ui->setupUi(this);

    QFont font = ui->nameVersionLabel->font();
    font.setPixelSize(24);
    font.setBold(true);
    ui->nameVersionLabel->setFont(font);

    ui->urlLabel->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse);

    // We have no help so remove the button from the buttons.
    setButtons(buttons() ^ KCModule::Help ^ KCModule::Default ^ KCModule::Apply);
}

Module::~Module()
{
    delete ui;
}

void Module::load()
{
    // NOTE: do not include globals, otherwise kdeglobals could provide values
    //       even though we only explicitly want them from our own config.
    KSharedConfig::Ptr config = KSharedConfig::openConfig(QStringLiteral("kcm-about-extendedrc"),
                                                          KConfig::NoGlobals);
    KConfigGroup cg = KConfigGroup(config, "General");

    QString logoPath = cg.readEntry("LogoPath", QString());
    QPixmap logo;
    if (logoPath.isEmpty())
        logo = QIcon::fromTheme(QStringLiteral("start-here-kde")).pixmap(128, 128);
    else
        logo = QPixmap(logoPath);
    ui->logoLabel->setPixmap(logo);
    QPixmap softwareLogo;
    QPixmap hardwareLogo;
    QPixmap graphicsLogo;
    QPixmap networkLogo;
    softwareLogo = QIcon::fromTheme(QStringLiteral("computer")).pixmap(48, 48);
    hardwareLogo = QIcon::fromTheme(QStringLiteral("cpu")).pixmap(48, 48);
    graphicsLogo = QIcon::fromTheme(QStringLiteral("video-display")).pixmap(48, 48);
    networkLogo = QIcon::fromTheme(QStringLiteral("preferences-system-network")).pixmap(48, 48);
    ui->softwareLogo->setPixmap(softwareLogo);
    ui->hardwareLogo->setPixmap(hardwareLogo);
    ui->graphicsLogo->setPixmap(graphicsLogo);
    ui->networkLogo->setPixmap(networkLogo);

    OSRelease os;
    // We allow overriding of the OS name for branding purposes.
    // For example OS Ubuntu may be rebranded as Kubuntu. Also Kubuntu Active
    // as a product brand is different from Kubuntu.
    QString distroName = cg.readEntry("Name", os.name);
    ui->nameVersionLabel->setText(QStringLiteral("%1").arg(distroName));

    // Set buildLabel from versionID
    QString versionId = cg.readEntry("BuildStamp", os.buildId);
    ui->buildLabel->setText(versionId);

    // Set OS Version label
    QString namedRelease;
    if (cg.hasKey("Version") && cg.hasKey("Release")) {
      namedRelease = QString("%1 %2 \"%3\"").arg(distroName,
                                                 cg.readEntry("Version"),
                                                 cg.readEntry("Release"));
    } else {
      namedRelease = os.prettyName;
    }
    ui->namedReleaseLabel->setText(namedRelease);

    QString url = cg.readEntry("Website", os.homeUrl);
    if (url.isEmpty())
        ui->urlLabel->hide();
    else
        ui->urlLabel->setText(QStringLiteral("<a href='%1'>%1</a>").arg(url));

    // Since Plasma version detection isn't based on a library query it can fail
    // in weird cases; instead of admiting defeat we simply hide everything :P
    QString plasma = plasmaVersion();
    if (plasma.isEmpty()) {
        ui->plasma->hide();
        ui->plasmaLabel->hide();
    } else {
        ui->plasmaLabel->setText(plasma);
    }

    ui->qtLabel->setText(qVersion());
    ui->frameworksLabel->setText(KCoreAddons::versionString());

    QString apps = appsVersion();
    if (!apps.isEmpty()) {
      ui->appsLabel->setText(apps);
    } else {
      ui->appsLabel->setHidden(true);
    }

    struct utsname utsName;
    if(uname(&utsName) != 0) {
        ui->kernel->hide();
        ui->kernelLabel->hide();
    } else
        ui->kernelLabel->setText(utsName.release);

    const int bits = QT_POINTER_SIZE == 8 ? 64 : 32;
    ui->bitsLabel->setText(i18nc("@label %1 is the CPU bit width (e.g. 32 or 64)",
                                 "%1-bit", QString::number(bits)));

    const QList<Solid::Device> list = Solid::Device::listFromType(Solid::DeviceInterface::Processor);
    ui->processor->setText(i18np("Processor:", "Processors:", list.count()));
    // Format processor string
    // Group by processor name
    QMap<QString, int> processorMap;
    Q_FOREACH(const Solid::Device &device, list) {
        const QString name = device.product();
        auto it = processorMap.find(name);
        if (it == processorMap.end()) {
            processorMap.insert(name, 1);
        } else {
            ++it.value();
        }
    }
    // Create a formatted list of grouped processors
    QStringList names;
    for (auto it = processorMap.constBegin(); it != processorMap.constEnd(); ++it) {
        const int count = it.value();
        QString name = it.key();
        name.replace(QStringLiteral("(TM)"), QChar(8482));
        name.replace(QStringLiteral("(R)"), QChar(174));
        name = name.simplified();
        names.append(QStringLiteral("%1 × %2").arg(count).arg(name));
    }
    ui->processorLabel->setText(names.join(QStringLiteral(", ")));
    if (ui->processorLabel->text().isEmpty()) {
        ui->processor->setHidden(true);
        ui->processorLabel->setHidden(true);
    }

    const qlonglong totalRam = calculateTotalRam();
    ui->memoryLabel->setText(totalRam > 0
                             ? i18nc("@label %1 is the formatted amount of system memory (e.g. 7,7 GiB)",
                                     "%1 of RAM", KFormat().formatByteSize(totalRam))
                             : i18nc("Unknown amount of RAM", "Unknown"));

     const qlonglong totalSwap = calculateTotalSwap();
     ui->swapAmount->setText(totalSwap > 0
                              ? i18nc("@label %1 is the formatted amount of system swap (e.g. 7,7 GiB)",
                                      "%1 of Swap", KFormat().formatByteSize(totalSwap))
                              : i18nc("Unknown amount of RAM", "Unknown"));


    OpenGLInfo openglInfo;
    ui->openglLabel->setText(openglInfo.openGLRenderer);
    ui->openglDriverLabel->setText(openglInfo.kwinDriver);
    ui->mesaVersionLabel->setText(openglInfo.mesaVersion);

    if (!openglInfo.displayServerVersion.isEmpty() && !openglInfo.displayServerVersion.isNull()) {
      ui->displayServerVersionLabel->setText(openglInfo.displayServerVersion);
    } else {
      ui->displayServerVersion->setHidden(true);
    }

    connect(NetworkManager::notifier(), &NetworkManager::Notifier::connectivityChanged, this, &Module::networkStatus);
}

void Module::save()
{
}

void Module::defaults()
{
}

QString Module::plasmaVersion() const
{
    const QStringList &filePaths = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation,
                                                             QStringLiteral("xsessions/plasma.desktop"));

    if (filePaths.length() < 1) {
        return QString();
    }

    // Despite the fact that there can be multiple desktop files we simply take
    // the first one as users usually don't have xsessions/ in their $HOME
    // data location, so the first match should (usually) be the only one and
    // reflect the plasma session run.
    KDesktopFile desktopFile(filePaths.first());
    return desktopFile.desktopGroup().readEntry("X-KDE-PluginInfo-Version", QString());
}

QString Module::appsVersion() const
{
  /* Grab KF5 & Qt5 info */
  QString kdeapps_version = QString::null;
  /* FIXME: unsafe, replace popen with QProcess? */
  FILE *fd1 = popen("dolphin --version", "r");
  if (fd1) {
      QTextStream is(fd1);
      while (!is.atEnd()) {
          QString line = is.readLine();
          if (line.startsWith("dolphin")) {
              kdeapps_version = line.section(' ', 1, 1);
          }
      }
  }
  if (fd1) {
      /* FIXME: this is hack to do not let QTextStream touch fd after closing
       * it. Prevents whole kio_sysinfo from crashing */
      pclose(fd1);
  }
  return kdeapps_version;
}

void Module::networkStatus()
{
  NetworkManager::Connectivity connectivity = NetworkManager::connectivity();
  switch (connectivity) {
     case NetworkManager::Full:
         ui->networkLabel->setText(i18n("You are online"));
         break;
     case NetworkManager::NoConnectivity:
         ui->networkLabel->setText(i18n("You are offline"));
         break;
    default:
        ui->networkLabel->setText(i18n("Network connectivity is unknown"));
        break;
  }
}
