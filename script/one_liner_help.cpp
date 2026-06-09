#include "one_liner_help.h"

#include <QDesktopServices>
#include <QDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMovie>
#include <QPixmap>
#include <QPushButton>
#include <QScrollArea>
#include <QSize>
#include <QString>
#include <QUrl>
#include <QVBoxLayout>
#include <QVector>
#include <QWidget>

namespace {

constexpr int kHelpBaseFontSize = 12;
constexpr int kHelpMediaWidth = 440;
constexpr int kHelpMediaHeight = 268;
constexpr int kHelpGifWidth = 488;
constexpr int kHelpGifHeight = 298;
constexpr int kHelpOuterHorizontalMargin = 22;
constexpr int kHelpOuterTopMargin = 20;
constexpr int kHelpOuterBottomMargin = 26;
constexpr int kHelpDialogExtraWidth = 112;
constexpr int kHelpDialogExtraHeight = 120;

struct HelpRowData {
    QString mediaPath;
    QString text;
    bool richText = false;
};

struct HelpSectionData {
    QString titleHtml;
    QVector<HelpRowData> rows;
    bool isCredits = false;
};

QFont baseUiFont()
{
    QFont font;
    font.setFamily(QStringLiteral("Microsoft YaHei UI"));
    font.setPixelSize(kHelpBaseFontSize);
    return font;
}

QString imagePath(const QString& name)
{
    return QStringLiteral("oneLinerImages:") + name;
}

QString iconPath(const QString& name)
{
    return QStringLiteral("oneLinerIcons:") + name;
}

QVector<HelpSectionData> helpSections()
{
    return {
        {
            QStringLiteral("<span style='color:#7dc6ff;'>基础用法</span>"),
            {
                { imagePath(QStringLiteral("01.gif")), QStringLiteral("选中的对象会实时显示在输入框下方，输入规则后预览会立即刷新。") },
            }
        },
        {
            QStringLiteral("<span style='color:#ffd27a;'>! # @ 符号</span>"),
            {
                { imagePath(QStringLiteral("02.gif")), QStringLiteral("[!] 复用旧名称，例如 side_!；支持中文输入法下的全角感叹号/问号等等效符号。") },
                { imagePath(QStringLiteral("03.gif")), QStringLiteral("[#] 数字序号：使用多个 `#` 控制补零位数，例如 `ctrl_##` → `01, 02, ...`；在标记末尾追加 `//N` 可以设置起始值，例如 `ctrl_##//3` → `03, 04, ...`。") },
                { imagePath(QStringLiteral("04.gif")), QStringLiteral("[@] 字母序号：支持两种模式。\n单个 `@`（可扩展进位）：产生 `A, B, ..., Z, AA, AB, ...`（溢出时左侧进位）。\n多个 `@`（固定宽度）：`@@` / `@@@` 保留精确字母位数，例如 `@@` 会生成 `AA, AB, ..., AZ, BA, ...`。\n可在标记后使用 `/` 提供起始模板与大小写控制，例如 `@@/Aa` 表示从 `Aa` 开始（模板中大写位表示该位为大写，小写位表示该位为小写）。若模板格式不合法，引擎会退化为把标记作为普通文本以避免不期望的前缀。") },
            }
        },
        {
            QStringLiteral("<span style='color:#89e5c8;'>-h -s -type 层级与类型</span>"),
            {
                { imagePath(QStringLiteral("05.gif")), QStringLiteral("[-h] 把当前选中对象及其子级加入候选列表，预览会按层级树分组显示。") },
                { QString(), QStringLiteral("[-h -s] 在层级模式下把 shape 一并加入候选列表。") },
                { QString(), QStringLiteral("[-type joint blendShape] 按类型过滤候选；如果当前没有候选，则等同 ls -type。") },
            }
        },
        {
            QStringLiteral("<span style='color:#ffb3a7;'>+ - -- old&gt;new 文本处理</span>"),
            {
                { imagePath(QStringLiteral("06.gif")), QStringLiteral("[+数字] 从前往后删字符；[-数字] 从后往前删字符；[--数字] 保留前 N 个字符。") },
                { QString(), QStringLiteral("[old>new] 对候选名字执行局部替换。") },
                { QString(), QStringLiteral("[Mesh A>Joint B] 可按顺序批量替换多个词；左右两侧也支持用英文逗号或中文逗号分隔。") },
            }
        },
        {
            QStringLiteral("<span style='color:#c7b3ff;'>* ? 通配符选择</span>"),
            {
                { QString(), QStringLiteral("直接输入 * 和 ? 使用 Maya 原生通配符匹配，回车会直接选中匹配对象；也支持中文输入法的全角问号。") },
                { QString(), QStringLiteral("右键菜单中的 [检索Shape对象] 只影响通配符结果，不影响 -type。") },
            }
        },
        {
            QStringLiteral("<span style='color:#9fd0ff;'>预览与历史</span>"),
            {
                { QString(), QStringLiteral("预览列表中的层级项会显示缩进圆点，双击任意项可把原始文本写回输入框。") },
                { QString(), QStringLiteral("上下方向键可切换当前 Maya 会话中的输入历史。") },
                { QString(), QStringLiteral("输入框右侧会显示输入法中英文状态；开启大写锁定时会额外显示大写图标。") },
            }
        },
        {
            QStringLiteral("<span style='color:#D97900;'>鸣谢</span>"),
            {
                { QString(), QStringLiteral("<div style='text-align:center; line-height:1.0;'><div style='font-size:22px; color:#ffffff; font-weight:700;'>Fauzan Syabana</div><div style='font-size:17px; color:#8fc7ff;'>zansyabana@gmail.com</div><div style='font-size:18px; color:#d7dde3;'>感谢原作者提供的思路</div><div style='font-size:17px; color:#c7d1db;'>开源协议：MIT</div></div>"), true },
            },
            true
        },
    };
}

QPushButton* createHelpLinkButton(const QString& iconName, const QString& url, const QString& toolTip, qreal scale, QWidget* parent)
{
    QPushButton* button = new QPushButton(parent);
    const int side = qRound(34 * scale);
    button->setFixedSize(side, side);
    button->setCursor(Qt::PointingHandCursor);
    button->setToolTip(toolTip);
    button->setIcon(QIcon(iconPath(iconName)));
    button->setIconSize(QSize(qRound(18 * scale), qRound(18 * scale)));
    button->setStyleSheet(QStringLiteral(
        "QPushButton {"
        " background: rgba(255,255,255,18);"
        " border: 1px solid rgba(255,255,255,30);"
        " border-radius: 10px;"
        "}"
        "QPushButton:hover { background: rgba(255,255,255,28); }"
        "QPushButton:pressed { background: rgba(255,255,255,40); }"));
    QObject::connect(button, &QPushButton::clicked, button, [url]() {
        QDesktopServices::openUrl(QUrl(url));
    });
    return button;
}

QWidget* createHelpMediaWidget(const QString& mediaPath, qreal scale, QWidget* parent)
{
    if (mediaPath.isEmpty()) {
        return nullptr;
    }

    QLabel* label = new QLabel(parent);
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet(QStringLiteral("QLabel { background: transparent; border: none; }"));

    if (mediaPath.endsWith(QStringLiteral(".gif"), Qt::CaseInsensitive)) {
        const QSize targetSize(qRound(kHelpGifWidth * scale), qRound(kHelpGifHeight * scale));
        QMovie probeMovie(mediaPath);
        probeMovie.jumpToFrame(0);
        QSize sourceSize = probeMovie.currentPixmap().size();
        if (!sourceSize.isValid()) {
            sourceSize = probeMovie.frameRect().size();
        }

        QMovie* movie = new QMovie(mediaPath, QByteArray(), label);
        if (sourceSize.isValid()) {
            const QSize scaledSize = sourceSize.scaled(targetSize, Qt::KeepAspectRatio);
            label->setFixedSize(scaledSize);
            movie->setScaledSize(scaledSize);
        } else {
            label->setFixedSize(targetSize);
        }
        label->setMovie(movie);
        movie->start();
    } else {
        const QSize targetSize(qRound(kHelpMediaWidth * scale), qRound(kHelpMediaHeight * scale));
        const QPixmap pixmap(mediaPath);
        const QPixmap scaledPixmap = pixmap.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        label->setFixedSize(scaledPixmap.size());
        label->setPixmap(scaledPixmap);
    }

    return label;
}

QWidget* createHelpSectionWidget(const HelpSectionData& section, qreal scale, QWidget* parent)
{
    QFrame* card = new QFrame(parent);
    card->setObjectName(QStringLiteral("helpCard"));
    card->setStyleSheet(section.isCredits
        ? QStringLiteral(
            "QFrame#helpCard {"
            " background: transparent;"
            " border: none;"
            "}"
            "QLabel[role='sectionTitle'] {"
            " color: #ffd27a;"
            " font-weight: 700;"
            "}"
            "QLabel[role='body'] {"
            " color: #d5dde5;"
            " line-height: 1.75;"
            "}"
        )
        : QStringLiteral(
            "QFrame#helpCard {"
            " background: rgba(19, 25, 34, 220);"
            " border: 1px solid rgba(99, 171, 219, 70);"
            " border-radius: 14px;"
            "}"
            "QLabel[role='sectionTitle'] {"
            " color: #f4f6f8;"
            " font-weight: 700;"
            "}"
            "QLabel[role='body'] {"
            " color: #d5dde5;"
            " line-height: 1.45;"
            "}"
        ));

    QVBoxLayout* layout = new QVBoxLayout(card);
    layout->setContentsMargins(qRound(10 * scale), qRound(10 * scale), qRound(10 * scale), qRound(10 * scale));
    layout->setSpacing(qRound(10 * scale));
    layout->setAlignment(section.isCredits ? Qt::AlignHCenter | Qt::AlignTop : Qt::AlignTop);

    QLabel* title = new QLabel(section.titleHtml, card);
    title->setProperty("role", QStringLiteral("sectionTitle"));
    title->setTextFormat(Qt::RichText);
    QFont titleFont = baseUiFont();
    titleFont.setPixelSize(qRound(section.isCredits ? 20 * scale : 16 * scale));
    titleFont.setBold(true);
    title->setFont(titleFont);
    title->setAlignment(section.isCredits ? Qt::AlignHCenter : Qt::AlignLeft);
    layout->addWidget(title);

    for (const HelpRowData& row : section.rows) {
        QVBoxLayout* rowLayout = new QVBoxLayout();
        rowLayout->setSpacing(qRound(8 * scale));
        rowLayout->setAlignment(section.isCredits ? Qt::AlignHCenter | Qt::AlignTop : Qt::AlignLeft | Qt::AlignTop);

        if (QWidget* mediaWidget = createHelpMediaWidget(row.mediaPath, scale, card)) {
            rowLayout->addWidget(mediaWidget, 0, section.isCredits ? Qt::AlignHCenter : Qt::AlignLeft);
        }

        const QStringList textLines = row.richText ? QStringList{row.text} : row.text.split(QChar(u'\n'), Qt::SkipEmptyParts);
        for (const QString& line : textLines) {
            QLabel* body = new QLabel(line, card);
            body->setProperty("role", QStringLiteral("body"));
            body->setWordWrap(true);
            body->setTextFormat(row.richText ? Qt::RichText : Qt::PlainText);
            QFont bodyFont = baseUiFont();
            bodyFont.setPixelSize(qRound(section.isCredits ? 15 * scale : 13 * scale));
            body->setFont(bodyFont);
            body->setAlignment(section.isCredits ? Qt::AlignHCenter : Qt::AlignLeft);
            rowLayout->addWidget(body, 0, section.isCredits ? Qt::AlignHCenter : Qt::AlignLeft);
        }

        layout->addLayout(rowLayout);
    }

    return card;
}

} // namespace

namespace OneLinerHelp {

QDialog* createHelpDialog(QWidget* parent, qreal scale)
{
    QDialog* dialog = new QDialog(parent, Qt::Window | Qt::WindowCloseButtonHint | Qt::WindowTitleHint);
    dialog->setAttribute(Qt::WA_DeleteOnClose, true);
    dialog->setWindowTitle(QStringLiteral("oneLiner 帮助"));
    dialog->setStyleSheet(QStringLiteral("QDialog { background: #111821; }"));

    QVBoxLayout* layout = new QVBoxLayout(dialog);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    QScrollArea* scrollArea = new QScrollArea(dialog);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet(QStringLiteral(
        "QScrollArea { background: transparent; border: none; }"
        "QScrollBar:vertical { width: 10px; background: transparent; margin: 12px 4px 12px 0; }"
        "QScrollBar:horizontal { height: 0px; background: transparent; }"
        "QScrollBar::handle:vertical { background: rgba(255,255,255,40); border-radius: 5px; min-height: 24px; }"
        "QScrollBar::handle:horizontal { background: transparent; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical, QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { height: 0; background: transparent; }"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal, QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal { width: 0; background: transparent; }"));
    layout->addWidget(scrollArea);

    QWidget* container = new QWidget(scrollArea);
    scrollArea->setWidget(container);

    QVBoxLayout* containerLayout = new QVBoxLayout(container);
    containerLayout->setAlignment(Qt::AlignTop);
    containerLayout->setContentsMargins(
        qRound(kHelpOuterHorizontalMargin * scale),
        qRound(kHelpOuterTopMargin * scale),
        qRound(kHelpOuterHorizontalMargin * scale),
        qRound(kHelpOuterBottomMargin * scale));
    containerLayout->setSpacing(qRound(10 * scale));

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setAlignment(Qt::AlignLeft);
    buttonLayout->setSpacing(qRound(10 * scale));
    buttonLayout->addWidget(createHelpLinkButton(QStringLiteral("bilibili.png"), QStringLiteral("https://space.bilibili.com/14857382"), QStringLiteral("打开 Bilibili"), scale, container));
    buttonLayout->addWidget(createHelpLinkButton(QStringLiteral("github.png"), QStringLiteral("https://github.com/ai12989757"), QStringLiteral("打开 GitHub"), scale, container));
    containerLayout->addLayout(buttonLayout);

    QLabel* title = new QLabel(QStringLiteral("重命名规则帮助"), container);
    title->setAlignment(Qt::AlignCenter);
    QFont titleFont = baseUiFont();
    titleFont.setPixelSize(qRound(22 * scale));
    titleFont.setBold(true);
    title->setFont(titleFont);
    title->setStyleSheet(QStringLiteral("color: #f4f6f8;"));
    containerLayout->addWidget(title);

    const QVector<HelpSectionData> sections = helpSections();
    for (const HelpSectionData& section : sections) {
        containerLayout->addWidget(createHelpSectionWidget(section, scale, container));
    }

    containerLayout->activate();
    layout->activate();

    const int fixedWidth = qRound((kHelpMediaWidth + kHelpDialogExtraWidth) * scale);
    const int minimumHeight = qRound(720 * scale);
    dialog->setFixedWidth(fixedWidth);
    dialog->setMinimumHeight(minimumHeight);
    dialog->resize(fixedWidth, minimumHeight);

    return dialog;
}

} // namespace OneLinerHelp