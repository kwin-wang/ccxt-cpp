#pragma once

#include <stdexcept>
#include <string>

namespace ccxt {

class Error : public std::runtime_error {
public:
    explicit Error(const std::string& message) : std::runtime_error(message) {}
};

class ExchangeError : public Error {
public:
    explicit ExchangeError(const std::string& message) : Error(message) {}
};

class AuthenticationError : public ExchangeError {
public:
    explicit AuthenticationError(const std::string& message) : ExchangeError(message) {}
};

class PermissionDenied : public AuthenticationError {
public:
    explicit PermissionDenied(const std::string& message) : AuthenticationError(message) {}
};

class AccountSuspended : public AuthenticationError {
public:
    explicit AccountSuspended(const std::string& message) : AuthenticationError(message) {}
};

class ArgumentsRequired : public ExchangeError {
public:
    explicit ArgumentsRequired(const std::string& message) : ExchangeError(message) {}
};

class BadRequest : public ExchangeError {
public:
    explicit BadRequest(const std::string& message) : ExchangeError(message) {}
};

class BadResponse : public ExchangeError {
public:
    explicit BadResponse(const std::string& message) : ExchangeError(message) {}
};

class NetworkError : public ExchangeError {
public:
    explicit NetworkError(const std::string& message) : ExchangeError(message) {}
};

class DDoSProtection : public NetworkError {
public:
    explicit DDoSProtection(const std::string& message) : NetworkError(message) {}
};

class RequestTimeout : public NetworkError {
public:
    explicit RequestTimeout(const std::string& message) : NetworkError(message) {}
};

class ExchangeNotAvailable : public NetworkError {
public:
    explicit ExchangeNotAvailable(const std::string& message) : NetworkError(message) {}
};

class InvalidNonce : public ExchangeError {
public:
    explicit InvalidNonce(const std::string& message) : ExchangeError(message) {}
};

class InvalidOrder : public ExchangeError {
public:
    explicit InvalidOrder(const std::string& message) : ExchangeError(message) {}
};

class OrderNotFound : public InvalidOrder {
public:
    explicit OrderNotFound(const std::string& message) : InvalidOrder(message) {}
};

class OrderNotCached : public InvalidOrder {
public:
    explicit OrderNotCached(const std::string& message) : InvalidOrder(message) {}
};

class CancelPending : public InvalidOrder {
public:
    explicit CancelPending(const std::string& message) : InvalidOrder(message) {}
};

class OrderImmediatelyFillable : public InvalidOrder {
public:
    explicit OrderImmediatelyFillable(const std::string& message) : InvalidOrder(message) {}
};

class OrderNotFillable : public InvalidOrder {
public:
    explicit OrderNotFillable(const std::string& message) : InvalidOrder(message) {}
};

class DuplicateOrderId : public InvalidOrder {
public:
    explicit DuplicateOrderId(const std::string& message) : InvalidOrder(message) {}
};

class InsufficientFunds : public ExchangeError {
public:
    explicit InsufficientFunds(const std::string& message) : ExchangeError(message) {}
};

class InvalidAddress : public ExchangeError {
public:
    explicit InvalidAddress(const std::string& message) : ExchangeError(message) {}
};

class AddressPending : public InvalidAddress {
public:
    explicit AddressPending(const std::string& message) : InvalidAddress(message) {}
};

class NotSupported : public ExchangeError {
public:
    explicit NotSupported(const std::string& message) : ExchangeError(message) {}
};

class RateLimitExceeded : public ExchangeError {
public:
    explicit RateLimitExceeded(const std::string& message) : ExchangeError(message) {}
};

} // namespace ccxt
