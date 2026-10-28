// SPDX-License-Identifier: GPL-2.0-or-later
#include "../frontend/utility/PenguinGPUUsage.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QDir>
#include <cstdlib>
void check(bool ok){if(!ok)std::abort();}
int main(int argc,char**argv){QCoreApplication app(argc,argv);QTemporaryDir dir;check(dir.isValid());auto path=dir.path()+"/renderD129/device";check(QDir().mkpath(path));auto file=path+"/gpu_busy_percent";
auto write=[&](QByteArray text){QFile f(file);check(f.open(QIODevice::WriteOnly));check(f.write(text)==text.size());};
for(auto value:{QByteArray("0\n"),QByteArray("100\n"),QByteArray("37\n")}){write(value);check(PenguinGPUUsage::ReadPercent(file)==value.trimmed().toInt());}
for(auto value:{QByteArray("-1"),QByteArray("101"),QByteArray("1.2"),QByteArray("abc"),QByteArray(""),QByteArray(64,'1')}){write(value);check(PenguinGPUUsage::ReadPercent(file)==-1);}
check(PenguinGPUUsage::ReadPercent(file+"missing")==-1);write("37\n");
PenguinGPUUsage monitor(dir.path());monitor.Configure("/dev/dri/renderD129","Selected adapter");check(monitor.Poll().percent==-1);QThreadPool::globalInstance()->waitForDone();auto sample=monitor.Poll();check(sample.percent==37&&sample.renderer=="Selected adapter");QThreadPool::globalInstance()->waitForDone();
monitor.Configure("unknown","Unsupported");check(monitor.Poll().percent==-1);monitor.Configure("/dev/dri/renderD129","Previous");monitor.Poll();monitor.Configure("unknown","New");QThreadPool::globalInstance()->waitForDone();check(monitor.Poll().percent==-1&&monitor.Poll().renderer=="New");
{PenguinGPUUsage transient(dir.path());transient.Configure("/dev/dri/renderD129","Temporary");transient.Poll();}QThreadPool::globalInstance()->waitForDone();
}
