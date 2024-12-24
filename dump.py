
import json
import os

def getAllExchanges():
    import ccxt
    return ccxt.exchanges

def dumpWsDesc(exchange_name:str):
    import ccxt.pro
    try:
        cex = getattr(ccxt.pro, exchange_name)
        descirpt = cex().describe()
        detail = str(descirpt)
        detail = detail.replace("'", '"')
        detail = detail.replace("True", "true")
        detail = detail.replace("False", "false")
        detail = detail.replace("None", "null")
        detail = detail.replace("<class ", "")
        detail = detail.replace(">", "")
        detail = detail.replace('don"t', "don't")
        detail = detail.replace('Amount"s', "Amount's")
        detail = detail.replace('AssetPair"s', "AssetPair's")
        detail = detail.replace(' e.g. "123.456"', " e.g. '123.456'")
        detail = detail.replace('We"ll', "We'll")
        detail = detail.replace('"from"', "'from'")
        detail = detail.replace('Buster"s', "Buster's")
        detail = detail.replace('it"s', "it's")
        detail = detail.replace('doesn"t ', "doesn't ")
        detail = detail.replace('user"s', "user's")
        detail = detail.replace('"usdt_btc"', "'usdt_btc'")
        detail = detail.replace('Can"t', "Can't")
        detail = detail.replace(' Users" Debt', " Users' Debt")
        detail = detail.replace('<bound', '"bound')
        # <bound method blofin.ping of ccxt.blofin()

        detail = detail.replace('(),', '()",') 
        detail = detail.replace('()}}', '()"}}')
        detail = detail.replace('ccxt.xt()},', 'ccxt.xt()"},')
        with open(f'{exchange_name}_ws.json', 'w') as f:
            f.write(detail)
        json_data = json.loads(detail)
        
        with open(f'config/{exchange_name}_ws.json', 'w') as f:
            json.dump(json_data, f, indent=2)
        
        os.remove(f'{exchange_name}_ws.json')

    except Exception as e:
        print(F"{exchange_name} error: {e}")
    
def dumpRestDesc(exchange_name:str):
    import ccxt
    try:
        cex = getattr(ccxt, exchange_name)
        descirpt = cex().describe()
        detail = str(descirpt)
        detail = detail.replace("'", '"')
        detail = detail.replace("True", "true")
        detail = detail.replace("False", "false")
        detail = detail.replace("None", "null")
        detail = detail.replace("<class ", "")
        detail = detail.replace(">", "")
        detail = detail.replace('don"t', "don't")
        detail = detail.replace('Amount"s', "Amount's")
        detail = detail.replace('AssetPair"s', "AssetPair's")
        detail = detail.replace(' e.g. "123.456"', " e.g. '123.456'")
        detail = detail.replace('We"ll', "We'll")
        detail = detail.replace('"from"', "'from'")
        detail = detail.replace('Buster"s', "Buster's")
        detail = detail.replace('it"s', "it's")
        detail = detail.replace('doesn"t ', "doesn't ")
        detail = detail.replace('user"s', "user's")
        detail = detail.replace('"usdt_btc"', "'usdt_btc'")
        detail = detail.replace('Can"t', "Can't")
        detail = detail.replace(' Users" Debt', " Users' Debt")

        with open(f'{exchange_name}_rest.json', 'w') as f:
            f.write(detail)
        json_data = json.loads(detail)
        
        with open(f'config/{exchange_name}_rest.json', 'w') as f:
            json.dump(json_data, f, indent=2)
        
        os.remove(f'{exchange_name}_rest.json')

    except Exception as e:
        print(F"{exchange_name} error: {e}")    

exchanges = getAllExchanges()
for exchange in exchanges:
    dumpWsDesc(exchange)