/*!
@file
Defines `boost::hana::Foldable`.

@copyright Louis Dionne 2015
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FOLDABLE_HPP
#define BOOST_HANA_FOLDABLE_HPP

#include <boost/hana/fwd/foldable.hpp>

#include <boost/hana/applicative.hpp>
#include <boost/hana/config.hpp>
#include <boost/hana/core/datatype.hpp>
#include <boost/hana/core/default.hpp>
#include <boost/hana/core/make.hpp>
#include <boost/hana/core/models.hpp>
#include <boost/hana/core/when.hpp>
#include <boost/hana/detail/by.hpp> // needed by xxx.by
#include <boost/hana/detail/create.hpp>
#include <boost/hana/detail/variadic/foldl1.hpp>
#include <boost/hana/detail/variadic/foldr1.hpp>
#include <boost/hana/detail/variadic/for_each.hpp>
#include <boost/hana/enumerable.hpp>
#include <boost/hana/functional/curry.hpp>
#include <boost/hana/functional/flip.hpp>
#include <boost/hana/functional/partial.hpp>
#include <boost/hana/fwd/sequence.hpp>
#include <boost/hana/integral_constant.hpp>
#include <boost/hana/logical.hpp>
#include <boost/hana/monad.hpp>
#include <boost/hana/monad_plus.hpp>
#include <boost/hana/monoid.hpp>
#include <boost/hana/orderable.hpp>
#include <boost/hana/ring.hpp>

#include <cstddef>
#include <type_traits>
#include <utility>


namespace boost { namespace hana {
    //////////////////////////////////////////////////////////////////////////
    // fold_left
    //////////////////////////////////////////////////////////////////////////
    template <typename T, typename>
    struct fold_left_impl : fold_left_impl<T, when<true>> { };

    template <typename T, bool condition>
    struct fold_left_impl<T, when<condition>> : default_ {
        // with state
        template <typename Xs, typename S, typename F>
        static constexpr decltype(auto) apply(Xs&& xs, S&& s, F&& f) {
            return hana::unpack(static_cast<Xs&&>(xs),
                hana::partial(
                    detail::variadic::foldl1,
                    static_cast<F&&>(f),
                    static_cast<S&&>(s)
                )
            );
        }

        // without state
        template <typename Xs, typename F>
        static constexpr decltype(auto) apply(Xs&& xs, F&& f) {
            return hana::unpack(static_cast<Xs&&>(xs),
                hana::partial(
                    detail::variadic::foldl1,
                    static_cast<F&&>(f)
                )
            );
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // fold_right
    //////////////////////////////////////////////////////////////////////////
    template <typename T, typename>
    struct fold_right_impl : fold_right_impl<T, when<true>> { };

    template <typename T, bool condition>
    struct fold_right_impl<T, when<condition>> : default_ {
        // with state
        template <typename Xs, typename S, typename F>
        static constexpr decltype(auto) apply(Xs&& xs, S&& s, F&& f) {
            return hana::unpack(static_cast<Xs&&>(xs),
                hana::partial(
                    detail::variadic::foldr,
                    static_cast<F&&>(f),
                    static_cast<S&&>(s)
                )
            );
        }

        // without state
        template <typename Xs, typename F>
        static constexpr decltype(auto) apply(Xs&& xs, F&& f) {
            return hana::unpack(static_cast<Xs&&>(xs),
                hana::partial(
                    detail::variadic::foldr,
                    static_cast<F&&>(f)
                )
            );
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // monadic_fold_left (with state)
    //////////////////////////////////////////////////////////////////////////
    namespace foldable_detail {
        struct end { };
        struct foldlM_helper {
            template <typename F, typename X, typename K, typename Z>
            constexpr decltype(auto) operator()(F&& f, X&& x, K&& k, Z&& z) const {
                return hana::chain(
                    static_cast<F&&>(f)(
                        static_cast<Z&&>(z),
                        static_cast<X&&>(x)
                    ),
                    static_cast<K&&>(k)
                );
            }
        };

        template <typename M, typename F>
        struct monadic_fold1_helper {
            F f;
            template <typename X, typename Y>
            constexpr decltype(auto) operator()(X&& x, Y&& y) const
            { return f(static_cast<X&&>(x), static_cast<Y&&>(y)); }
            template <typename X>
            constexpr decltype(auto) operator()(X&& x, end) const
            { return hana::lift<M>(static_cast<X&&>(x)); }
            template <typename Y>
            constexpr decltype(auto) operator()(end, Y&& y) const
            { return hana::lift<M>(static_cast<Y&&>(y)); }
        };
    }

    template <typename T, typename>
    struct monadic_fold_left_impl : monadic_fold_left_impl<T, when<true>> { };

    template <typename T, bool condition>
    struct monadic_fold_left_impl<T, when<condition>> : default_ {
        template <typename M, typename Xs, typename S, typename F>
        static constexpr decltype(auto) apply(Xs&& xs, S&& s, F&& f) {
            return hana::fold_right(
                static_cast<Xs&&>(xs),
                hana::lift<M>,
                hana::curry<3>(hana::partial(
                    foldable_detail::foldlM_helper{}, static_cast<F&&>(f)
                ))
            )(static_cast<S&&>(s));
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // monadic_fold_left (without state)
    //////////////////////////////////////////////////////////////////////////
    template <typename T, typename>
    struct monadic_fold_left_nostate_impl
        : monadic_fold_left_nostate_impl<T, when<true>>
    { };

    template <typename T, bool condition>
    struct monadic_fold_left_nostate_impl<T, when<condition>> : default_ {
        template <typename M, typename Xs, typename F>
        static constexpr decltype(auto) apply(Xs&& xs, F&& f) {
            using namespace foldable_detail;
            using G = monadic_fold1_helper<M, typename std::decay<F>::type>;
            decltype(auto) result = hana::monadic_fold<M>.left(
                static_cast<Xs&&>(xs),
                end{},
                G{static_cast<F&&>(f)}
            );

            static_assert(!std::is_same<
                decltype(result),
                decltype(hana::lift<M>(end{}))
            >{},
            "hana::monadic_fold<M>.left(xs, f) requires xs to be non-empty");
            return result;
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // monadic_fold_right (with state)
    //////////////////////////////////////////////////////////////////////////
    namespace foldable_detail {
        struct foldrM_helper {
            template <typename F, typename K, typename X, typename Z>
            constexpr decltype(auto) operator()(F&& f, K&& k, X&& x, Z&& z) const {
                return hana::chain(
                    static_cast<F&&>(f)(
                        static_cast<X&&>(x),
                        static_cast<Z&&>(z)
                    ),
                    static_cast<K&&>(k)
                );
            }
        };
    }

    template <typename T, typename>
    struct monadic_fold_right_impl : monadic_fold_right_impl<T, when<true>> { };

    template <typename T, bool condition>
    struct monadic_fold_right_impl<T, when<condition>> : default_ {
        template <typename M, typename Xs, typename S, typename F>
        static constexpr decltype(auto) apply(Xs&& xs, S&& s, F&& f) {
            return hana::fold_left(
                static_cast<Xs&&>(xs),
                hana::lift<M>,
                hana::curry<3>(hana::partial(
                    foldable_detail::foldrM_helper{}, static_cast<F&&>(f)
                ))
            )(static_cast<S&&>(s));
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // monadic_fold_right (without state)
    //////////////////////////////////////////////////////////////////////////
    template <typename T, typename>
    struct monadic_fold_right_nostate_impl
        : monadic_fold_right_nostate_impl<T, when<true>>
    { };

    template <typename T, bool condition>
    struct monadic_fold_right_nostate_impl<T, when<condition>> : default_ {
        template <typename M, typename Xs, typename F>
        static constexpr decltype(auto) apply(Xs&& xs, F&& f) {
            using namespace foldable_detail;
            using G = monadic_fold1_helper<M, typename std::decay<F>::type>;
            decltype(auto) result = hana::monadic_fold<M>.right(
                static_cast<Xs&&>(xs),
                end{},
                G{static_cast<F&&>(f)}
            );

            static_assert(!std::is_same<
                decltype(result),
                decltype(hana::lift<M>(end{}))
            >{},
            "hana::monadic_fold<M>.right(xs, f) requires xs to be non-empty");
            return result;
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // for_each
    //////////////////////////////////////////////////////////////////////////
    template <typename T, typename>
    struct for_each_impl : for_each_impl<T, when<true>> { };

    namespace foldable_detail {
        template <typename F>
        struct for_each_fun {
            F f;
            template <typename X>
            constexpr int operator()(int /*ignore*/, X&& x) const& {
                f(static_cast<X&&>(x));
                return 0;
            }
#ifndef BOOST_HANA_CONFIG_CONSTEXPR_MEMBER_FUNCTION_IS_CONST
            template <typename X>
            constexpr int operator()(int /*ignore*/, X&& x) & {
                f(static_cast<X&&>(x));
                return 0;
            }
#endif
            template <typename X>
            constexpr int operator()(int /*ignore*/, X&& x) && {
                std::move(f)(static_cast<X&&>(x));
                return 0;
            }
        };

        template <typename T, bool = is_default<unpack_impl<T>>{}()>
        struct for_each_helper {
            template <typename Xs, typename F>
            static constexpr void apply(Xs&& xs, F&& f) {
                hana::unpack(static_cast<Xs&&>(xs),
                    hana::partial(detail::variadic::for_each,
                                  static_cast<F&&>(f))
                );
            }
        };

        template <typename T>
        struct for_each_helper<T, true> {
            template <typename Xs, typename F>
            static constexpr void apply(Xs&& xs, F&& f) {
                // we ignore the state all the way
                hana::fold_left(static_cast<Xs&&>(xs), 0,
                    detail::create<for_each_fun>{}(static_cast<F&&>(f))
                );
            }
        };
    }

    template <typename T, bool condition>
    struct for_each_impl<T, when<condition>>
        : foldable_detail::for_each_helper<T>, default_
    { };

    //////////////////////////////////////////////////////////////////////////
    // length
    //////////////////////////////////////////////////////////////////////////
    template <typename T, typename>
    struct length_impl : length_impl<T, when<true>> { };

    namespace detail {
        struct argn {
            template <typename ...Xs>
            constexpr auto operator()(Xs const& ...) const
            { return hana::size_t<sizeof...(Xs)>; }
        };
    }

    template <typename T, bool condition>
    struct length_impl<T, when<condition>> : default_ {
        template <typename Xs>
        static constexpr auto apply(Xs const& xs) {
            return hana::unpack(xs, detail::argn{});
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // minimum (with a custom predicate)
    //////////////////////////////////////////////////////////////////////////
    namespace foldable_detail {
        template <typename Pred>
        struct minpred {
            Pred pred;
            template <typename X, typename Y>
            constexpr decltype(auto) operator()(X&& x, Y&& y) const& {
                decltype(auto) r = pred(x, y);
                return hana::if_(static_cast<decltype(r)&&>(r),
                    static_cast<X&&>(x),
                    static_cast<Y&&>(y)
                );
            }
#ifndef BOOST_HANA_CONFIG_CONSTEXPR_MEMBER_FUNCTION_IS_CONST
            template <typename X, typename Y>
            constexpr decltype(auto) operator()(X&& x, Y&& y) & {
                decltype(auto) r = pred(x, y);
                return hana::if_(static_cast<decltype(r)&&>(r),
                    static_cast<X&&>(x),
                    static_cast<Y&&>(y)
                );
            }
#endif
            template <typename X, typename Y>
            constexpr decltype(auto) operator()(X&& x, Y&& y) && {
                decltype(auto) r = std::move(pred)(x, y);
                return hana::if_(static_cast<decltype(r)&&>(r),
                    static_cast<X&&>(x),
                    static_cast<Y&&>(y)
                );
            }
        };
    }

    template <typename T, typename>
    struct minimum_pred_impl : minimum_pred_impl<T, when<true>> { };

    template <typename T, bool condition>
    struct minimum_pred_impl<T, when<condition>> : default_ {
        template <typename Xs, typename Pred>
        static constexpr decltype(auto) apply(Xs&& xs, Pred&& pred) {
            return hana::fold_left(static_cast<Xs&&>(xs),
                detail::create<foldable_detail::minpred>{}(
                    static_cast<Pred&&>(pred)
                )
            );
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // minimum (without a custom predicate)
    //////////////////////////////////////////////////////////////////////////
    template <typename T, typename>
    struct minimum_impl : minimum_impl<T, when<true>> { };

    template <typename T, bool condition>
    struct minimum_impl<T, when<condition>> : default_ {
        template <typename Xs>
        static constexpr decltype(auto) apply(Xs&& xs)
        { return hana::minimum(static_cast<Xs&&>(xs), hana::less); }
    };

    //////////////////////////////////////////////////////////////////////////
    // maximum (with a custom predicate)
    //////////////////////////////////////////////////////////////////////////
    namespace foldable_detail {
        template <typename Pred>
        struct maxpred {
            Pred pred;
            template <typename X, typename Y>
            constexpr decltype(auto) operator()(X&& x, Y&& y) const& {
                decltype(auto) r = pred(x, y);
                return hana::if_(static_cast<decltype(r)&&>(r),
                    static_cast<Y&&>(y),
                    static_cast<X&&>(x)
                );
            }
#ifndef BOOST_HANA_CONFIG_CONSTEXPR_MEMBER_FUNCTION_IS_CONST
            template <typename X, typename Y>
            constexpr decltype(auto) operator()(X&& x, Y&& y) & {
                decltype(auto) r = pred(x, y);
                return hana::if_(static_cast<decltype(r)&&>(r),
                    static_cast<Y&&>(y),
                    static_cast<X&&>(x)
                );
            }
#endif
            template <typename X, typename Y>
            constexpr decltype(auto) operator()(X&& x, Y&& y) && {
                decltype(auto) r = std::move(pred)(x, y);
                return hana::if_(static_cast<decltype(r)&&>(r),
                    static_cast<Y&&>(y),
                    static_cast<X&&>(x)
                );
            }
        };
    }

    template <typename T, typename>
    struct maximum_pred_impl : maximum_pred_impl<T, when<true>> { };

    template <typename T, bool condition>
    struct maximum_pred_impl<T, when<condition>> : default_ {
        template <typename Xs, typename Pred>
        static constexpr decltype(auto) apply(Xs&& xs, Pred&& pred) {
            return hana::fold_left(static_cast<Xs&&>(xs),
                detail::create<foldable_detail::maxpred>{}(
                    static_cast<Pred&&>(pred)
                )
            );
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // maximum (without a custom predicate)
    //////////////////////////////////////////////////////////////////////////
    template <typename T, typename>
    struct maximum_impl : maximum_impl<T, when<true>> { };

    template <typename T, bool condition>
    struct maximum_impl<T, when<condition>> : default_ {
        template <typename Xs>
        static constexpr decltype(auto) apply(Xs&& xs)
        { return hana::maximum(static_cast<Xs&&>(xs), hana::less); }
    };

    //////////////////////////////////////////////////////////////////////////
    // sum
    //////////////////////////////////////////////////////////////////////////
    template <typename T, typename>
    struct sum_impl : sum_impl<T, when<true>> { };

    template <typename T, bool condition>
    struct sum_impl<T, when<condition>> : default_ {
        template <typename M, typename Xs>
        static constexpr decltype(auto) apply(Xs&& xs) {
            return hana::fold_left(static_cast<Xs&&>(xs), zero<M>(), plus);
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // product
    //////////////////////////////////////////////////////////////////////////
    template <typename T, typename>
    struct product_impl : product_impl<T, when<true>> { };

    template <typename T, bool condition>
    struct product_impl<T, when<condition>> : default_ {
        template <typename R, typename Xs>
        static constexpr decltype(auto) apply(Xs&& xs) {
            return hana::fold_left(static_cast<Xs&&>(xs), one<R>(), mult);
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // count_if
    //////////////////////////////////////////////////////////////////////////
    namespace foldable_detail {
        template <typename Pred>
        struct countpred {
            Pred pred;
            template <typename Counter, typename X>
            constexpr decltype(auto) operator()(Counter&& counter, X&& x) const& {
                return hana::if_(pred(static_cast<X&&>(x)),
                    hana::succ(counter),
                    counter
                );
            }
#ifndef BOOST_HANA_CONFIG_CONSTEXPR_MEMBER_FUNCTION_IS_CONST
            template <typename Counter, typename X>
            constexpr decltype(auto) operator()(Counter&& counter, X&& x) & {
                return hana::if_(pred(static_cast<X&&>(x)),
                    hana::succ(counter),
                    counter
                );
            }
#endif
            template <typename Counter, typename X>
            constexpr decltype(auto) operator()(Counter&& counter, X&& x) && {
                return hana::if_(std::move(pred)(static_cast<X&&>(x)),
                    hana::succ(counter),
                    counter
                );
            }
        };
    }

    template <typename T, typename>
    struct count_if_impl : count_if_impl<T, when<true>> { };

    template <typename T, bool condition>
    struct count_if_impl<T, when<condition>> : default_ {
        template <typename Xs, typename Pred>
        static constexpr decltype(auto) apply(Xs&& xs, Pred&& pred) {
            return hana::fold_left(static_cast<Xs&&>(xs), size_t<0>,
                detail::create<foldable_detail::countpred>{}(
                    static_cast<Pred&&>(pred)
                )
            );
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // count
    //////////////////////////////////////////////////////////////////////////
    template <typename T, typename>
    struct count_impl : count_impl<T, when<true>> { };

    template <typename T, bool condition>
    struct count_impl<T, when<condition>> : default_ {
        template <typename Xs, typename Value>
        static constexpr decltype(auto) apply(Xs&& xs, Value&& value) {
            return hana::count_if(static_cast<Xs&&>(xs),
                hana::equal.to(static_cast<Value&&>(value)));
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // unpack
    //////////////////////////////////////////////////////////////////////////
    template <typename T, typename>
    struct unpack_impl : unpack_impl<T, when<true>> { };

    template <typename T, bool condition>
    struct unpack_impl<T, when<condition>> : default_ {
        template <typename Xs, typename F>
        static constexpr decltype(auto) apply(Xs&& xs, F&& f) {
            return hana::fold_left(
                static_cast<Xs&&>(xs),
                static_cast<F&&>(f),
                partial
            )();
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // models
    //////////////////////////////////////////////////////////////////////////
    template <typename T>
    struct models_impl<Foldable, T>
        : _integral_constant<bool,
            !is_default<fold_left_impl<T>>{}() ||
            !is_default<unpack_impl<T>>{}()
        >
    { };

    //////////////////////////////////////////////////////////////////////////
    // Model for builtin arrays
    //////////////////////////////////////////////////////////////////////////
    template <typename T, std::size_t N>
    struct unpack_impl<T[N]> {
        template <typename Xs, typename F, std::size_t ...i>
        static constexpr decltype(auto)
        unpack_helper(Xs&& xs, F&& f, std::index_sequence<i...>) {
            return static_cast<F&&>(f)(
                static_cast<Xs&&>(xs)[i]...
            );
        }

        template <typename Xs, typename F>
        static constexpr decltype(auto) apply(Xs&& xs, F&& f) {
            return unpack_helper(
                static_cast<Xs&&>(xs),
                static_cast<F&&>(f),
                std::make_index_sequence<N>{}
            );
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // Automatic Foldable -> Sequence conversion
    //////////////////////////////////////////////////////////////////////////
    template <typename S, typename F>
    struct to_impl<S, F, when<_models<Sequence, S>{}() &&
                              _models<Foldable, F>{}()>>
        : embedding<_models<Sequence, F>{}()>
    {
        template <typename Xs>
        static constexpr decltype(auto) apply(Xs&& xs)
        { return hana::unpack(static_cast<Xs&&>(xs), hana::make<S>); }
    };
}} // end namespace boost::hana

#endif // !BOOST_HANA_FOLDABLE_HPP
