#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>

// 模拟文件到期提醒场景
void simulateReminder(QDateTime expirationDate, int reminderHours, int intervalMinutes) {
    qDebug() << "=== 提醒测试 ===";
    qDebug() << "到期时间:" << expirationDate.toString("yyyy-MM-dd HH:mm:ss");
    qDebug() << "提前提醒时间:" << reminderHours << "小时";
    qDebug() << "提醒间隔:" << intervalMinutes << "分钟";
    
    QDateTime now = QDateTime::currentDateTime();
    qDebug() << "当前时间:" << now.toString("yyyy-MM-dd HH:mm:ss");
    
    int reminderSeconds = reminderHours * 3600;
    int intervalSeconds = intervalMinutes * 60;
    
    // 检查是否在提醒范围内
    if (now >= expirationDate.addSecs(-reminderSeconds) && now < expirationDate) {
        qDebug() << "✓ 在提醒范围内";
        qDebug() << "提醒间隔（秒）:" << intervalSeconds;
        
        // 计算距离到期时间还有多少秒
        int secondsToExpiration = now.secsTo(expirationDate);
        qDebug() << "距离到期时间还有:" << secondsToExpiration << "秒（" << secondsToExpiration / 3600.0 << "小时）";
        
        // 计算下一次提醒时间
        QDateTime nextReminderTime = now.addSecs(intervalSeconds);
        qDebug() << "下一次提醒时间:" << nextReminderTime.toString("yyyy-MM-dd HH:mm:ss");
    } else {
        qDebug() << "✗ 不在提醒范围内";
    }
    
    qDebug() << "=== 测试结束 ===\n";
}

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);
    
    // 模拟场景：文件在今天00:00到期，提前24小时提醒，间隔1小时
    QDateTime expirationDate = QDateTime::currentDateTime().addDays(1).setTime(QTime(0, 0, 0));
    simulateReminder(expirationDate, 24, 60);
    
    // 模拟场景：文件在明天00:00到期，提前48小时提醒，间隔30分钟
    expirationDate = QDateTime::currentDateTime().addDays(2).setTime(QTime(0, 0, 0));
    simulateReminder(expirationDate, 48, 30);
    
    return 0;
}