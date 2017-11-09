#include "bitcoinunits.h"
#include "optionsmodel.h"
#include <QStringList>
#include <QLocale>

WalletModel *walletModel;

BitcoinUnits::BitcoinUnits(QObject *parent, WalletModel *wModel):
        QAbstractListModel(parent),
        unitlist(availableUnits())
{
    walletModel = wModel;
}

QList<BitcoinUnits::Unit> BitcoinUnits::availableUnits()
{
    QList<BitcoinUnits::Unit> unitlist;
    unitlist.append(VRC);
    unitlist.append(mVRC);
    unitlist.append(uVRC);
    unitlist.append(USD);
    return unitlist;
}

bool BitcoinUnits::valid(int unit)
{
    switch(unit)
    {
    case VRC:
    case mVRC:
    case uVRC:
    case USD:
        return true;
    default:
        return false;
    }
}

QString BitcoinUnits::name(int unit)
{
    switch(unit)
    {
    case VRC: return QString("VRC");
    case mVRC: return QString("mVRC");
    case uVRC: return QString::fromUtf8("μVRC");
    case USD:  return QString("USD");
    default: return QString("???");
    }
}

QString BitcoinUnits::description(int unit)
{
    switch(unit)
    {
    case VRC: return QString("VeriCoins");
    case mVRC: return QString("Milli-VeriCoins (1 / 1,000)");
    case uVRC: return QString("Micro-VeriCoins (1 / 1,000,000)");
    case USD: return QString("US Dollar");
    default: return QString("???");
    }
}

qint64 BitcoinUnits::factor(int unit)
{
    switch(unit)
    {
    case VRC:
    case USD:  
        return 100000000;
    case mVRC: return 100000;
    case uVRC: return 100;
    default:   return 100000000;
    }
}

int BitcoinUnits::amountDigits(int unit)
{
    switch(unit)
    {
    case VRC:
    case USD: 
        return 8; // 21,000,000 (# digits, without commas)
    case mVRC: return 11; // 21,000,000,000
    case uVRC: return 14; // 21,000,000,000,000
    default: return 0;
    }
}

int BitcoinUnits::maxdecimals(int unit)
{
    switch(unit)
    {
        case VRC: return 8;
        case mVRC: return 5;
        case uVRC:
        case USD: 
            return 2;
        default: return 0;
    }
}

int BitcoinUnits::decimals(int unit)
{
    // USD should only be two
    if(unit == USD)
        return maxdecimals(unit);

    if(walletModel && walletModel->getOptionsModel())
    {
        if (walletModel->getOptionsModel()->getDecimalPoints() > maxdecimals(unit))
            return maxdecimals(unit);
        else
            return walletModel->getOptionsModel()->getDecimalPoints();
    }
    else
    {
        return maxdecimals(unit);
    }
}

QString BitcoinUnits::format(int unit, qint64 n, bool fPlus, bool fHideAmounts)
{
    return formatMaxDecimals(unit, n, decimals(unit), fPlus, fHideAmounts);
}

QString BitcoinUnits::formatMaxDecimals(int unit, qint64 n, int decimals, bool fPlus, bool fHideAmounts, bool fPretty)
{
    // Note: not using straight sprintf here because we do NOT want
    // localized number formatting.
    if(!valid(unit))
        return QString(); // Refuse to format invalid unit
    qint64 coin = factor(unit);
    qint64 n_abs = (n > 0 ? n : -n);
    qint64 quotient = n_abs / coin;
    qint64 remainder = n_abs % coin;

    // force USD to two decimals to save room
    if(unit == USD)
        decimals = 2;

    QString quotient_str = QString::number(quotient);
    if (fPretty)
        quotient_str = QString("%L1").arg(quotient);
    QString remainder_str = QString::number(remainder).rightJustified(maxdecimals(unit), '0').left(decimals);
        
        // Pad zeros after remainder up to number of decimals
    for (int i = remainder_str.size(); i < decimals; ++i)
        remainder_str.append("0");

    if(fHideAmounts)
    {
        quotient_str.replace(QRegExp("[0-9]"),"*");
        remainder_str.replace(QRegExp("[0-9]"),"*");
    }

    if (n < 0)
        quotient_str.insert(0, '-');
    else if (fPlus && n > 0)
        quotient_str.insert(0, '+');

    QString strResult;
    if (remainder_str.size())
        strResult = quotient_str + QString(".") + remainder_str;
    else
        strResult = quotient_str;

    // append USD to the vericoin amount
    if(unit == USD)
        return appendUSD(strResult);
    else
        return strResult;
    
}

QString BitcoinUnits::formatFee(int unit, qint64 n, bool fPlus)
{
    // Note: not using straight sprintf here because we do NOT want
    // localized number formatting.
    if(!valid(unit))
        return QString(); // Refuse to format invalid unit
    qint64 coin = factor(unit);
    qint64 n_abs = (n > 0 ? n : -n);
    qint64 quotient = n_abs / coin;
    qint64 remainder = n_abs % coin;
    QString quotient_str = QString("%L1").arg(quotient);
    QString remainder_str = QString::number(remainder).rightJustified(maxdecimals(unit), '0');

    // Right-trim excess zeros after the decimal point
    int nTrim = 0;
    for (int i = remainder_str.size()-1; i>=2 && (remainder_str.at(i) == '0'); --i)
        ++nTrim;
    remainder_str.chop(nTrim);

    if (n < 0)
        quotient_str.insert(0, '-');
    else if (fPlus && n > 0)
        quotient_str.insert(0, '+');
    
    QString strResult;
    if (remainder_str.size())
        strResult = quotient_str + QString(".") + remainder_str;
    else
        strResult = quotient_str;

    // append USD to the vericoin amount
    if(unit == USD)
        return appendUSD(strResult);
    else
        return strResult;

}

// Calling this function will return the maximum number of decimals based on the options setting.
QString BitcoinUnits::formatWithUnit(int unit, qint64 amount, bool plussign, bool hideamounts)
{
    // USD is represented with $ and
    // we also want to see VRC amount for reference
    // so switch the option from string "USD" to "VRC"
    QString strUnitName;
    if (unit == USD) 
        strUnitName = name(VRC);
    else 
        strUnitName = name(unit);

    return format(unit, amount, plussign, hideamounts) + QString(" ") + strUnitName;
}

// Calling this function with maxdecimals(unit) will return the maximum number of decimals.
QString BitcoinUnits::formatWithUnitWithMaxDecimals(int unit, qint64 amount, int decimals, bool plussign, bool hideamounts)
{
    // USD is represented with $ and
    // we also want to see VRC amount for reference
    // so switch the option from string "USD" to "VRC"
    QString strUnitName;
    if (unit == USD) 
        strUnitName = name(VRC);
    else 
        strUnitName = name(unit);

    return formatMaxDecimals(unit, amount, decimals, plussign, hideamounts) + QString(" ") + strUnitName;
}

// Calling this function will return the maximum number of decimals for a fee.
QString BitcoinUnits::formatWithUnitFee(int unit, qint64 amount, bool plussign)
{
    return formatFee(unit, amount, plussign) + QString(" ") + name(unit);
}

bool BitcoinUnits::parse(int unit, const QString &value, qint64 *val_out)
{
    if(!valid(unit) || value.isEmpty())
        return false; // Refuse to parse invalid unit or empty string
    int num_decimals = maxdecimals(unit);
    QStringList parts = value.split(".");

    if(parts.size() > 2)
    {
        return false; // More than one dot
    }
    QString whole = parts[0];
    QString decimals;

    if(parts.size() > 1)
    {
        decimals = parts[1];
    }
    if(decimals.size() > num_decimals)
    {
        return false; // Exceeds max precision
    }
    bool ok = false;
    QString str = whole + decimals.leftJustified(num_decimals, '0');

    if(str.size() > 18)
    {
        return false; // Longer numbers will exceed 63 bits
    }
    qint64 retvalue = str.toLongLong(&ok);
    if(val_out)
    {
        *val_out = retvalue;
    }
    return ok;
}

int BitcoinUnits::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return unitlist.size();
}

QVariant BitcoinUnits::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    if(row >= 0 && row < unitlist.size())
    {
        Unit unit = unitlist.at(row);
        switch(role)
        {
        case Qt::EditRole:
        case Qt::DisplayRole:
            return QVariant(name(unit));
        case Qt::ToolTipRole:
            return QVariant(description(unit));
        case UnitRole:
            return QVariant(static_cast<int>(unit));
        }
    }
    return QVariant();
}

QString BitcoinUnits::appendUSD(QString strVrcAmount){
  
    
    // strip out the non-number stuff
    QString strOriginal = strVrcAmount;
    strVrcAmount.remove('+');
    strVrcAmount.remove('-');
    strVrcAmount.remove(',');

    // convert to dollar amount to calculate string
    float dDollarAmount  = strVrcAmount.toFloat()*dUSDRate;
    float dDollarDecimal = dDollarAmount - (int)dDollarAmount;

    // if the dollar amount is large we will shorten with abbrievations
    QString strDollarAmount = QString::number((int)dDollarAmount);
    if (dDollarAmount >= 1000 && dDollarAmount < 1000000){
        strDollarAmount = QString::number(dDollarAmount/1000.0f).mid(0,5);
        strDollarAmount.append("K");
    }
    else if (dDollarAmount >= 1000000 && dDollarAmount < 1000000000){
        strDollarAmount = QString::number(dDollarAmount/1000000.0f).mid(0,5);
        strDollarAmount.append("M");
    }
    else if (dDollarAmount >= 1000000000){
        strDollarAmount = QString::number(dDollarAmount/1000000000.0f).mid(0,5);
        strDollarAmount.append("B");
    }

    // if there is less than 1000 than just take the dollar amount
    // as is and add the standard decimal
    QString strDollarDecimal = "";
    if (dDollarAmount < 1000 && dDollarDecimal != 0){
        strDollarDecimal = QString::number(dDollarDecimal);
        strDollarDecimal.remove(0, 2);
        strDollarDecimal = strDollarDecimal.mid(0, 2);
        strDollarDecimal = QString(".") + strDollarDecimal;
    }

    // if USD rate was never updated indicate this with ?'s
    // otherwise build a dollar amount string and append it to the VRC amount
    if (dUSDRate == -1)
        strOriginal.append(QString(" ($\?\?\?.\?\?)"));
    else
        strOriginal.append(QString(" ($") + strDollarAmount + strDollarDecimal + QString(")"));

    return strOriginal;
}